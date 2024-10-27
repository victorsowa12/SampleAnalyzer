#include "SDI12Analyzer.h"
#include "SDI12AnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

SDI12Analyzer::SDI12Analyzer()
:	Analyzer2(),
	mSettings(),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( &mSettings );
}

SDI12Analyzer::~SDI12Analyzer()
{
	KillThread();
}

void SDI12Analyzer::SetupResults()
{
	// SetupResults is called each time the analyzer is run. Because the same instance can be used for multiple runs, we need to clear the results each time.
	mResults.reset(new SDI12AnalyzerResults( this, &mSettings ));
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings.mInputChannel );
}

void SDI12Analyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();

	mSerial = GetAnalyzerChannelData( mSettings.mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();

	U32 samples_per_bit = mSampleRateHz / mSettings.mBitRate;
	U32 samples_to_first_center_of_first_data_bit = U32( 1.5 * double( mSampleRateHz ) / double( mSettings.mBitRate ) );
	U32 samples_from_end_of_break_to_end_of_marking_periods = U32( double( mSampleRateHz ) * double( mSettings.mMarkingPeriodMs ) / 1000.0 );

	for( ; ; )
	{
		bool parity_error = false;
		bool framing_error = false;
		U8 data = 0;
		// SDI12 is LSB first so we will start the mask with the least significant bit
		U8 mask = 0x01;

		// We need to see if we have a break period on our hands
		U64 break_start_sample = mSerial->GetSampleNumber();
		U64 break_end_sample = mSerial->GetSampleOfNextEdge();
		// Check to see if this is a break period
		double break_period_ms = 1000.0 * double(break_end_sample - break_start_sample) / double(mSampleRateHz);
		if (fabs(break_period_ms - mSettings.mBreakPeriodMs) <= 0.4)
		{
			mSerial->AdvanceToNextEdge(); //falling edge -- to the end of the break period
			// we just got a break period so lets save a break frame!
			Frame break_frame;
			break_frame.mType = BreakPeriodFT;
			break_frame.mFlags = 0;
			break_frame.mStartingSampleInclusive = break_start_sample;
			break_frame.mEndingSampleInclusive = break_end_sample;

			mResults->AddFrame( break_frame );
			mResults->CommitResults();
			ReportProgress( break_frame.mEndingSampleInclusive );

			// Now there is the marking period, so lets advance to the next edge (aka to the start of the start bit)
			mSerial->AdvanceToNextEdge();
			// And lets save a marking frame!
			Frame marking_frame;
			marking_frame.mType = MarkingPeriodFT;
			marking_frame.mFlags = 0;
			marking_frame.mStartingSampleInclusive = break_end_sample;
			marking_frame.mEndingSampleInclusive = mSerial->GetSampleNumber();

			mResults->AddFrame( marking_frame );
			mResults->CommitResults();
			ReportProgress( marking_frame.mEndingSampleInclusive );
		}

		U64 starting_sample = mSerial->GetSampleNumber();

		mSerial->Advance( samples_to_first_center_of_first_data_bit );

		// SDI12 has only 7 data bits
		for( U32 i=0; i<7; i++ )
		{
			//let's put a dot exactly where we sample this bit:
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings.mInputChannel );

			// SDI12 is inverted, so we will record a 1 when the line is low
			if( mSerial->GetBitState() == BIT_LOW )
				data |= mask;

			mSerial->Advance( samples_per_bit );

			// Since it is LSB first we need to shift the mask to the left
			mask = mask << 1;
		}

		// Now we have to deal with the parity bit
		// There is one parity bit, so we can advance by one bit
		// SDI12 uses even parity so ne need to check if it is even/odd parity
		parity_error = false;
		bool is_even = AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( data ) );
		if( is_even == true )
		{
				if( mSerial->GetBitState() != BIT_HIGH ) // we expect a high bit, to keep the parity even.
						parity_error = true;
		}
		else
		{
				if( mSerial->GetBitState() != BIT_LOW ) // we expect a low bit, to force parity even.
						parity_error = true;
		}
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Square, mSettings.mInputChannel );

		// now we must determine if there is a framing error.
		framing_error = false;
		mSerial->Advance( samples_per_bit );
		if( mSerial->GetBitState() != BIT_LOW )
		{
				framing_error = true;
		}

		if( framing_error == true )
		{
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings.mInputChannel );

		}
		else
		{
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings.mInputChannel );
		}

		mSerial->Advance( samples_per_bit / 2 ); // Advance to the end of the stop bit
		U64 end_of_sample = mSerial->GetSampleNumber();

		//we have a byte to save.
		Frame frame;
		frame.mType = DataFT;
		frame.mData1 = data;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = starting_sample;
		frame.mEndingSampleInclusive = end_of_sample;

		mResults->AddFrame( frame );
		mResults->CommitResults();
		ReportProgress( frame.mEndingSampleInclusive );

		mSerial->AdvanceToNextEdge(); //Go to the next high edge - may be the start of the next break period or the next start bit

	}
}

bool SDI12Analyzer::NeedsRerun()
{
	return false;
}

U32 SDI12Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), &mSettings );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SDI12Analyzer::GetMinimumSampleRateHz()
{
	return mSettings.mBitRate * 4;
}

const char* SDI12Analyzer::GetAnalyzerName() const
{
	return "SDI12 Analyzer";
}

const char* GetAnalyzerName()
{
	return "SDI12 Analyzer";
}

Analyzer* CreateAnalyzer()
{
	return new SDI12Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}