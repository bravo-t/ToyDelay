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

typedef std::vector<CCSLUT> CCSLUTS;

void
CCSDriverData::initVoltageWaveforms(const CCSGroup& luts)
{
  const CCSLUTS& lutTables = luts.tables();
  _voltageWaveforms.reserve(lutTbles.size());
  for (const CCSLUT& lutTable : lutTables) {
    const Waveform& volWave = calcVoltageWaveform(lutTable);
    _voltageWaveforms.push_back(volWage);
  }
}

static inline size_t 
indexByLoad(const CCSGroup& ccsData, 
            size_t beginIdx, size_t endIdx, double load)
{
  const CCSLUTS& luts = ccsData.tables();
  size_t lower = beginIdx+1;
  size_t upper = endIdx-2;
  if (load <= luts[lower].outputLoad()) {
    return beginIdx;
  }
  if (load >= luts[upper].outputLoad()) {
    return upper;
  }
  size_t idx = 0;
  while (upper - lower > 1) {
    idx = (lower + upper) >> 1;
    if (luts[idx].outputLoad() > load) {
      upper = idx;
    } else {
      lower = idx;
    }
  }
  if (luts[idx].outputLoad() > load) {
    --idx;
  }
  return idx;
}

static inline double 
getInputTransitionForIndex(const CCSLUTS& luts, 
                           const std::vector<size_t>& searchPos, 
                           size_t idx)
{
  return luts[searchPos[idx]].inputTransition();
}

static inline void 
indexByTransition(const CCSGroup& ccsData, double inputTran, 
                  size_t& beginIdx, size_t& endIdx)
{
  const CCSLUTS& luts = ccsData.tables();
  const std::vector<size_t>& searchPos = ccsData.searchSteps();
  
  size_t lower = 1;
  size_t upper = searchPos.size()-2;
  if (inputTran <= getInputTransitionForIndex(luts, searchPos, lower)) {
    return searchPos[0];
  }
  if (inputTran >= getInputTransitionForIndex(luts, searchPos, upper)) {
    return searchPos[upper];
  }
  size_t idx = 0;
  while (upper - lower > 1) {
    idx = (lower + upper) >> 1;
    if (getInputTransitionForIndex(luts, searchPos, idx) > inputTran) {
      upper = idx;
    } else {
      lower = idx;
    }
  }
  if (getInputTransitionForIndex(luts, searchPos, idx) > inputTran) {
    --idx;
  }
  return searchPos[idx];
}

double
CCSDriverData::referenceTime(double inputTran) const
{

}

}