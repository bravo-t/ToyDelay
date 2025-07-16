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

const CCSGroup&
CCSDriverData::ccsGroup() const 
{
  LUTType type = LUTType::RiseCurrent;
  if (isRise == false) {
    type = LUTType::FallCurrent;
  }
  return _arc->getCurrent(type);
}

const CCSLUT&
CCSDriverData::ccsTable(size_t index) const
{
  return ccsGroup().tables()[index];
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

static inline size_t
indexByTransition(const CCSGroup& ccsData, double inputTran) 
{
  const CCSLUTS& luts = ccsData.tables();
  const std::vector<size_t>& searchPos = ccsData.searchSteps();
  
  size_t lower = 1;
  size_t upper = searchPos.size()-2;
  if (inputTran <= getInputTransitionForIndex(luts, searchPos, lower)) {
    return 0;
  }
  if (inputTran >= getInputTransitionForIndex(luts, searchPos, upper)) {
    return upper;
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
  return idx;
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
    beginIdx = searchPos[0];
    endIdx = searchPos[1];
  }
  if (inputTran >= getInputTransitionForIndex(luts, searchPos, upper)) {
    beginIdx = searchPos[upper];
    endIdx = searchPos[upper+1];
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
  beginIdx = searchPos[idx];
  endIdx = searchPos[idx+1];
}

double
CCSDriverData::referenceTime(double inputTran) const
{
  size_t idx1 = 0;
  size_t idx2 = 0;
  indexByTransition(ccsGroup(), inputTran, idx1, idx2);
  const CCSLUT& tbl1 = ccsTable(idx1);
  const CCSLUT& tbl2 = ccsTable(idx2);
  double rt1 = tbl1.referenceTime();
  double tran1 = tbl1.inputTransition();
  double rt2 = tbl2.referenceTime();
  double tran2 = tbl2.inputTransition();
  double k = (rt1 - rt2) / (tran1 - tran2);
  double b = rt1 - k*tran1;
  return k*inputTran + b;
}

Waveform
CCSDriverData::driverWaveform(double inputTran, double outputLoad) const
{

}

Waveform
CCSDriverData::interpolateVoltageWaveforms(double inputTran, double outputLoad, 
                                           const std::vector<double>& timeSteps) const
{

}

}