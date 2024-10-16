#include "SDI12AnalyzerSettings.h"
#include <AnalyzerHelpers.h>


SDI12AnalyzerSettings::SDI12AnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 1200 ),
	mBreakMs( 12 ),
	mMarkingMs( 9 ),
	mInputChannelInterface(),
	mBitRateInterface(),
	mBreakMsInterface(),
	mMarkingMsInterface()
{
	mInputChannelInterface.SetTitleAndTooltip( "Serial", "Standard SDI12 Analyzer" );
	mInputChannelInterface.SetChannel( mInputChannel );

	mBitRateInterface.SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface.SetMax( 1200 );
	mBitRateInterface.SetMin( 1200 );
	mBitRateInterface.SetInteger( mBitRate );

	mBreakMsInterface.SetTitleAndTooltip( "Break Time (ms)", "Specify the break time in milliseconds." );
	mBreakMsInterface.SetMax( 100 );
	mBreakMsInterface.SetMin( 12 );
	mBreakMsInterface.SetInteger( mBreakMs );

	mMarkingMsInterface.SetTitleAndTooltip( "Marking Time (ms)", "Specify the marking time in milliseconds." );
	mMarkingMsInterface.SetMax( 100 );
	mMarkingMsInterface.SetMin( 9 );
	mMarkingMsInterface.SetInteger( mMarkingMs );

	AddInterface( &mInputChannelInterface );
	AddInterface( &mBitRateInterface );
	AddInterface( &mBreakMsInterface );
	AddInterface( &mMarkingMsInterface );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

SDI12AnalyzerSettings::~SDI12AnalyzerSettings()
{
}

bool SDI12AnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface.GetChannel();
	mBitRate = mBitRateInterface.GetInteger();
	mBreakMs = mBreakMsInterface.GetInteger();
	mMarkingMs = mMarkingMsInterface.GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "SDI12 Analyzer", true );

	return true;
}

void SDI12AnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface.SetChannel( mInputChannel );
	mBitRateInterface.SetInteger( mBitRate );
	mBreakMsInterface.SetInteger( mBreakMS );
	mMarkingMsInterface.SetInteger( mMarkingMS );
}

void SDI12AnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	text_archive >> mBreakMs;
	text_archive >> mMarkingMs;

	ClearChannels();
	AddChannel( mInputChannel, "SDI12 Analyzer", true );

	UpdateInterfacesFromSettings();
}

const char* SDI12AnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;
	text_archive << mBreakMs;
	text_archive << mMarkingMs;

	return SetReturnString( text_archive.GetString() );
}
