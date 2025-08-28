#include <cassert>
#include "CSMDriver.h"
#include "Debug.h"
#include "Plotter.h"

namespace NA {

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
    std::vector<double> newEffCaps;
    newEffCaps.push_back(0); /// effCap @ T=0
    for (size_t i=1; i<_timeSteps.size(); ++i) {
      newEffCaps.push_back(calcEffectiveCap(simResult, _timeSteps[i-1], _timeSteps[i]));
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

double
CSMDriver::calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const
{
  if (simResult.empty()) {
    return totalConnectedCap(_driverArc, _ckt, _isMax, _isRise);
  } else {
    const Device& driverSource = _ckt->device(_driverArc->driverSourceId());
    /// Add new function in SimResult to calculate charge during a time period
    double periodCharge = simResult.chargeBetween(driverSource, timeStart, timeEnd);
    double startVoltage = simResult.nodeVoltage(driverSource._posNode, timeStart);
    double endVoltage = simResult.nodeVoltage(driverSource._posNode, timeEnd);
    double newEffCap = std::abs(periodCharge / (endVoltage - startVoltage));
    return newEffCap;
  }
}

Waveform
assembleDriverWaveform(const CCSDriverData& driverData, const std::vector<double>& timeSteps)
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