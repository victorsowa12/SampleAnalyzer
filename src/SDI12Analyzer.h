#ifndef SDI12_ANALYZER_H
#define SDI12_ANALYZER_H

#include <Analyzer.h>
#include "SDI12AnalyzerSettings.h"
#include "SDI12AnalyzerResults.h"
#include "SDI12SimulationDataGenerator.h"
#include <memory>

class ANALYZER_EXPORT SDI12Analyzer : public Analyzer2
{
public:
	SDI12Analyzer();
	virtual ~SDI12Analyzer();

	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	SDI12AnalyzerSettings mSettings;
	std::unique_ptr<SDI12AnalyzerResults> mResults;
	AnalyzerChannelData* mSerial;

	SDI12SimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SDI12_ANALYZER_H
