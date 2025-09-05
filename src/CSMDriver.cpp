#include <cassert>
#include <algorithm>
#include "CSMDriver.h"
#include "Debug.h"
#include "Plotter.h"
#include "LibData.h"

namespace NA {

void
initVoltageRegions(bool extend, bool isRise, const LibData* libData, 
                   std::vector<double>& voltageRegions, double termVoltage,
                   double& vth, double& vl, double& vh)
{
  double fullVoltage = libData->voltage();
  vth = 0;
  vh = 0; 
  vl = 0;
  if (isRise) {
    vth = libData->riseDelayThres() / 100.0 * fullVoltage;
    vh = libData->riseTransitionHighThres() / 100.0 * fullVoltage;
    vl = libData->riseTransitionLowThres() / 100.0 * fullVoltage;
  } else {
    vth = -libData->fallDelayThres() / 100.0 * fullVoltage;
    vh = -libData->fallTransitionHighThres() / 100.0 * fullVoltage;
    vl = -libData->fallTransitionLowThres() / 100.0 * fullVoltage;
  }
  voltageRegions.clear();
  voltageRegions.push_back(0.0f);
  if (extend) {
    for (size_t i=0; i<10; ++i) {
      if (isRise) {
        voltageRegions.push_back(0.1*fullVoltage*(i+1));
      } else {
        voltageRegions.push_back(-0.1*fullVoltage*(i+1));
      }
    }
  }
  voltageRegions.push_back(vl);
  voltageRegions.push_back(vh);
  voltageRegions.push_back(vth);
  voltageRegions.push_back(termVoltage);
  std::sort(voltageRegions.begin(), voltageRegions.end());
  voltageRegions.erase(std::unique(voltageRegions.begin(), voltageRegions.end()), voltageRegions.end());
  if (isRise == false) {
    std::reverse(voltageRegions.begin(), voltageRegions.end());
  }
}

void
CSMDriverData::init(const CCSArc* arc, bool isRise)
{
  _isRise = isRise;
  _arc = arc;
  LUTType type = LUTType::RiseCurrent;
  if (isRise == false) {
    type = LUTType::FallCurrent;
  }
  initVoltageWaveforms(_arc->getCurrent(type));
  initVoltageRegions(false, isRise, arc->owner(), _voltageSteps, _termVoltage, _vth, _vl, _vh);
}

const CCSGroup&
CSMDriverData::ccsGroup() const 
{
  LUTType type = LUTType::RiseCurrent;
  if (_isRise == false) {
    type = LUTType::FallCurrent;
  }
  return _arc->getCurrent(type);
}

const CCSLUT&
CSMDriverData::ccsTable(size_t index) const
{
  return ccsGroup().tables()[index];
}

Waveform
calcVoltageWaveform(const CCSLUT& lutData) 
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
    voltage += dv;
    voltages.addPoint(currentT, voltage);
  }
  return voltages;
}

typedef std::vector<CCSLUT> CCSLUTS;

void
CSMDriverData::initVoltageWaveforms(const CCSGroup& luts)
{
  const CCSLUTS& lutTables = luts.tables();
  _voltageWaveforms.reserve(lutTables.size());
  _termVoltage = 1e99;
  if (_isRise == false) {
    _termVoltage = -1e99;
  }
  for (const CCSLUT& lutTable : lutTables) {
    const Waveform& volWave = calcVoltageWaveform(lutTable);
    _voltageWaveforms.push_back(volWave);
    double lastVol = volWave.data().back()._value;
    if (_isRise) {
      _termVoltage = std::min(_termVoltage, lastVol);
    } else {
      _termVoltage = std::max(_termVoltage, lastVol);
    }
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
CSMDriverData::referenceTime(double inputTran) const
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
CSMDriverData::driverWaveform(double inputTran, double outputLoad) const
{
  std::vector<double> timeSteps;
  for (double v : _voltageSteps) {
    double t = timeAtVoltage(inputTran, outputLoad, v);
    timeSteps.push_back(t);
  }
  return interpolateVoltageWaveforms(inputTran, outputLoad, timeSteps);
}

std::vector<double>
CSMDriverData::timeSteps(double inputTran, double outputLoad) const
{
  std::vector<double> timeSteps;
  for (double v : _voltageSteps) {
    double t = timeAtVoltage(inputTran, outputLoad, v);
    timeSteps.push_back(t);
  }
  return timeSteps;
}

std::vector<double>
CSMDriverData::timeSteps(double inputTran, const std::vector<double>& effCaps) const
{
  assert(effCaps.size() == 1 || effCaps.size() == _voltageSteps.size());
  if (effCaps.size() == 1) {
    return timeSteps(inputTran, effCaps[0]);
  }
  std::vector<double> timeSteps;
  for (size_t i=0; i<effCaps.size(); ++i) {
    double cap = effCaps[i];
    double v = _voltageSteps[i];
    double t = timeAtVoltage(inputTran, cap, v);
    timeSteps.push_back(t);
  }
  return timeSteps;
}

static void
findBoundingIndex(const CCSGroup& groupData, double inputTran, double outputLoad, 
                  size_t& idx1, size_t& idx2, size_t& idx3, size_t& idx4)
{
  size_t searchPos1 = indexByTransition(groupData, inputTran);
  const std::vector<size_t>& searchPos = groupData.searchSteps();
  assert(searchPos1 != searchPos.size()-2);
  size_t searchPos2 = searchPos1 + 1;
  size_t beginIdx1 = searchPos[searchPos1];
  size_t endIdx1 = searchPos[searchPos2];
  idx1 = indexByLoad(groupData, beginIdx1, endIdx1, outputLoad);
  idx2 = idx1 + 1;
  size_t beginIdx2 = searchPos[searchPos2];
  size_t endIdx2 = searchPos[searchPos2+1];
  idx3 = indexByLoad(groupData, beginIdx2, endIdx2, outputLoad);
  idx4 = idx3 + 1;
}

static Waveform
interpolateWaveform(const Waveform& v11, const Waveform& v12, 
                    const Waveform& v21, const Waveform& v22,
                    double inputTran1, double outputLoad1, 
                    double inputTran2, double outputLoad2,
                    double inputTran, double outputLoad, 
                    const std::vector<double>& timeSteps)
{
  Waveform voltages;
  for (double time : timeSteps) {
    double q11 = v11.value(time);
    double q12 = v12.value(time);
    double q21 = v21.value(time);
    double q22 = v22.value(time);
    double q = bilinearInterpolate(inputTran1, outputLoad1, inputTran2, outputLoad2, 
                                   q11, q12, q21, q22, inputTran, outputLoad);
    voltages.addPoint(time, q);
  }
  return voltages;
}

Waveform
CSMDriverData::interpolateVoltageWaveforms(double inputTran, double outputLoad, 
                                           const std::vector<double>& timeSteps) const
{
  const CCSGroup& groupData = ccsGroup();
  size_t idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;
  findBoundingIndex(groupData, inputTran, outputLoad, idx1, idx2, idx3, idx4);
  const std::vector<CCSLUT>& luts = groupData.tables();
  const CCSLUT& lut1 = luts[idx1];
  const CCSLUT& lut4 = luts[idx4];
  const Waveform& v11 = _voltageWaveforms[idx1];
  const Waveform& v12 = _voltageWaveforms[idx2];
  const Waveform& v21 = _voltageWaveforms[idx3];
  const Waveform& v22 = _voltageWaveforms[idx4];
  return interpolateWaveform(v11, v12, v21, v22, 
                             lut1.inputTransition(), lut1.outputLoad(), 
                             lut4.inputTransition(), lut4.outputLoad(), 
                             inputTran, outputLoad, timeSteps);
}

static double
timeAtVoltage(const Waveform& v11, const Waveform& v12, 
              const Waveform& v21, const Waveform& v22,
              double inputTran1, double outputLoad1, 
              double inputTran2, double outputLoad2,
              double inputTran, double outputLoad, double voltage)
{
  double t11 = v11.measure(voltage);
  double t12 = v12.measure(voltage);
  double t21 = v21.measure(voltage);
  double t22 = v22.measure(voltage);
  return bilinearInterpolate(inputTran1, outputLoad1, inputTran2, outputLoad2, 
                             t11, t12, t21, t22, inputTran, outputLoad);
}

double
CSMDriverData::timeAtVoltage(double inputTran, double outputLoad, double voltage) const
{
  const CCSGroup& groupData = ccsGroup();
  size_t idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;
  findBoundingIndex(groupData, inputTran, outputLoad, idx1, idx2, idx3, idx4);
  const std::vector<CCSLUT>& luts = groupData.tables();
  const CCSLUT& lut1 = luts[idx1];
  const CCSLUT& lut4 = luts[idx4];
  const Waveform& v11 = _voltageWaveforms[idx1];
  const Waveform& v12 = _voltageWaveforms[idx2];
  const Waveform& v21 = _voltageWaveforms[idx3];
  const Waveform& v22 = _voltageWaveforms[idx4];
  return ::NA::timeAtVoltage(v11, v12, v21, v22, 
                             lut1.inputTransition(), lut1.outputLoad(), 
                             lut4.inputTransition(), lut4.outputLoad(), 
                             inputTran, outputLoad, voltage);
}

double
totalConnectedCap(const CellArc* driverArc, const Circuit* ckt, bool isMax, bool isRise) 
{
  double totalCap = 0;
  size_t vsrcId = driverArc->driverSourceId();
  const std::vector<const Device*>& connDevs = ckt->traceDevice(vsrcId);
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor) {
      if (dev->_isInternal) {
        const std::vector<CellArc*>& loadArcs = ckt->cellArcsOfDevice(dev);
        double arcCap = 0;
        if (isMax == false) {
          arcCap = 1e99;
        }
        for (const CellArc* loadArc : loadArcs) {
          if (isMax) {
            arcCap = std::max(arcCap, loadArc->fixedLoadCap(isRise));
          } else { 
            arcCap = std::min(arcCap, loadArc->fixedLoadCap(isRise));
          }
        }
        totalCap += arcCap;
      } else {
        totalCap += dev->_value;
      }
    }
  }
  return totalCap;
}


/// Init will calculate every data based on previous iteration of simulation, 
/// Including timeSteps, effCaps of each time step, and driver waveform
void 
CSMDriver::init(Circuit* ckt, const CellArc* driverArc, bool isRise, bool isMax)
{
  _isRise = isRise;
  _isMax = isMax;
  _driverArc = driverArc;
  _ckt = ckt;
  _driverData.init(driverArc->ccsData(), isRise);
  _inputTran = _driverArc->inputTransition(_ckt);
  //_effCaps.push_back(totalConnectedCap(_driverArc, _ckt));
  //_timeSteps = _driverData.timeSteps(_inputTran, _effCaps[0]);
}

std::vector<double> 
timeRegions(const Waveform& driverVoltage, const std::vector<double>& voltageRegions)
{
  std::vector<double> timeRegion;
  if (voltageRegions[0] != 0) {
    timeRegion.push_back(0);
  }
  for (double v : voltageRegions) {
    double t = driverVoltage.measure(v);
    assert(t != 1e99);
    timeRegion.push_back(t);
  }
  return timeRegion;
}

bool
isVectorEqual(const std::vector<double>& a, 
              const std::vector<double>& b) 
{
  double eps = 1e-6;
  if (a.size() != b.size()) {
    return false;
  } 
  for (size_t i=0; i<a.size(); ++i) {
    double diff = a[i] - b[i];
    if (std::abs((diff/a[i])) > eps || std::abs(diff/b[i])> eps) {
      return false;
    }
  }
  return true;
}

bool 
CSMDriver::updateDriverData(const SimResult& simResult)
{ 
  if (simResult.empty()) {
    _effCaps.push_back(totalConnectedCap(_driverArc, _ckt, _isMax, _isRise));
    _timeSteps = _driverData.timeSteps(_inputTran, _effCaps[0]);
  } else {
    //updateTimeSteps(simResult);
    std::vector<double> newEffCaps;
    newEffCaps.push_back(0); /// effCap @ T=0
    for (size_t i=1; i<_timeSteps.size(); ++i) {
      double periodStart = _timeSteps[i-1];
      double periodEnd = _timeSteps[i];
      double c = calcEffectiveCap(simResult, periodStart, periodEnd);
      newEffCaps.push_back(c);
    }
    if (isVectorEqual(_effCaps, newEffCaps)) {
      return true;
    }
    std::vector<double> newTimeSteps = _driverData.timeSteps(_inputTran, newEffCaps);
    bool iterate = false;
    while (iterate) {
      if (isVectorEqual(_effCaps, newEffCaps) == false) {
        _effCaps.swap(newEffCaps);
        _timeSteps.swap(newTimeSteps);
        newEffCaps.clear();
        newTimeSteps.clear();
        newEffCaps.push_back(0);
        for (size_t i=1; i<_timeSteps.size(); ++i) {
          newEffCaps.push_back(calcEffectiveCap(simResult, _timeSteps[i-1], _timeSteps[i]));
        }
        newTimeSteps = _driverData.timeSteps(_inputTran, newEffCaps);
      } else {
        break;
      }
    }
    _effCaps.swap(newEffCaps);
    _timeSteps.swap(newTimeSteps);
  }
  return false;
}

void 
CSMDriver::updateTimeSteps(const SimResult& simResult)
{
  const Device& driverSource = _ckt->device(_driverArc->driverSourceId());
  size_t driverNodeId = driverSource._posNode;
  const Waveform& driverWaveform = simResult.nodeVoltageWaveform(driverNodeId);
  const std::vector<double>& voltageRegions = _driverData.voltageRegions();
  _timeSteps = timeRegions(driverWaveform, voltageRegions);
}

double
CSMDriver::calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const
{
  if (simResult.empty()) {
    return totalConnectedCap(_driverArc, _ckt, _isMax, _isRise);
  } else {
    const Device& driverSource = _ckt->device(_driverArc->driverSourceId());
    /// Add new function in SimResult to calculate charge during a time period
    double periodCharge = simResult.chargeBetween(driverSource, timeStart, timeEnd);
    if (periodCharge == 0) {
      return 0;
    }
    double startVoltage = simResult.nodeVoltage(driverSource._posNode, timeStart);
    double endVoltage = simResult.nodeVoltage(driverSource._posNode, timeEnd);
    double newEffCap = std::abs(periodCharge / (endVoltage - startVoltage));
    return newEffCap;
  }
}

Waveform
assembleDriverWaveform(const CSMDriverData& driverData, const std::vector<double>& timeSteps)
{
  const std::vector<double>& voltageRegions = driverData.voltageRegions();
  assert(voltageRegions.size() == timeSteps.size());
  Waveform waveform;
  for (size_t i=0; i<timeSteps.size(); ++i) {
    waveform.addPoint(timeSteps[i], voltageRegions[i]);
  }
  return waveform;
}

bool
CSMDriver::updateCircuit(const SimResult& simResult)
{
  bool converged = updateDriverData(simResult);
  const Waveform& driverWaveform = assembleDriverWaveform(_driverData, _timeSteps);
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: Driver pin waveform:\n");
    Plotter::plotWaveforms({driverWaveform});
  }

  /// update driver data
  const Device& driverSource = _ckt->device(_driverArc->driverSourceId());
  PWLValue& driverData = _ckt->PWLData(driverSource);
  driverData._time.clear();
  driverData._value.clear();
  for (const auto& p : driverWaveform.data()) {
    driverData._time.push_back(p._time);
    driverData._value.push_back(p._value);
  }
  return converged;
}



}