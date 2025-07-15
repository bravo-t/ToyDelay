#include "CCSData.h"

namespace NA {

CCSDriverData::CCSDriverData(const CCSArc* arc, bool isRise)
: _isRise(isRise), _arc(arc)
{
	LUTType type = LUTType::RiseCurrent;
	if (isRise == false) {
		type = LUTType::FallCurrent;
	}
	initVoltageWaveforms(_arc->getCurrent(type));
}

Waveform
calcVoltageWaveform(const CCSLUT& lutData, bool isRise) 
{
	Waveform voltages;
	double cap = lutData.outputLoad();
	const std::vector<double>& timeSteps = lutData.times();
	const std::vector<double>& currents = lutData.values();
	if (timeSteps.empty()) {
		return voltages;
	}
	double voltage = 0;
	voltages.addPoint(timeSteps[0], 0);
	for (size_t i=1; i<timeSteps.size(); ++i) {
		double prevT = timeSteps[i-1];
		double prevI = currents[i-1];
		double currentT = timeSteps[i];
		double currentI = currents[i];
		double dt = currentT - prevT;
		double dv = (prevI + currentI) / 2 * dt / cap;
		if (isRise == false) {
			dv = -dv;
		}
		voltage += dv;
		voltages.addPoint(currentT, voltage);
	}
	return voltages;
}

void
CCSDriverData::initVoltageWaveforms(const CCSGroup& luts)
{
	const std::vector<CCSLUT>& lutTables = luts.tables();
	_voltageWaveforms.reserve(lutTbles.size());
	for (const CCSLUT& lutTable : lutTables) {
		const Waveform& volWave = calcVoltageWaveform(lutTable);
		_voltageWaveforms.push_back(volWage);
	}
}

double
CCSDriverData::referenceTime(double inputTran) const
{

}

}