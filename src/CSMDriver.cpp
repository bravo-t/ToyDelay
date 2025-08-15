#include "CSMDriver.h"

namespace NA {

double
totalConnectedCap(const CellArc* driverArc, const Circuit* ckt) 
{
  double totalCap = 0;
  size_t vsrcId = driverArc->driverSourceId();
  const std::vector<const Device*>& connDevs = ckt->traceDevice(vsrcId);
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor) {
      totalCap += dev->_value;
    }
  }
  return totalCap;
}


/// Init will calculate every data based on previous iteration of simulation, 
/// Including timeSteps, effCaps of each time step, and driver waveform
void 
CSMDriver::init(Circuit* ckt, const CellArc* driverArc, bool isRise)
{
  _driverArc = driverArc;
  _ckt = ckt;
  _driverData.init(driverArc->ccsData(), isRise);
  _inputTran = _driverArc->inputTransition(_ckt);
  _effCaps.push_back(totalConnectedCap(_driverArc, _ckt));
  _timeSteps = _driverData.timeSteps(_inputTran, _effCaps[0]);
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

void
CSMDriver::update(const SimResult& simResult)
{ 
  _effCaps.clear();
  _effCaps.push_back(0); /// effCap @ T=0
  for (size_t i=1; i<timeRegion.size(); ++i) {
    _effCaps.push_back(calcEffectiveCap(simResult, timeRegion[i-1], timeRegion[i]));
  }
  const std::vector<double> newTimeSteps = _driverData.timeSteps(_inputTran, _effCaps);
  _timeSteps.clear();
  /// compare _timeSteps and newTimeSteps, iterate until stablizes?
  /// or just assign?
  _timeSteps = newTimeSteps;
}

double
CSMDriver::calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const
{
  if (simResult.empty()) {
    return totalConnectedCap(_driverArc, _ckt);
  } else {
    if (simResult.currentTime() < timeEnd) {
      return _effCap;
    } else {
      const Device& driverSource = _ckt->device(_driverArc->driverSourceId);
      double totalCharge = std::abs(simResult.totalCharge(driverSource));
      double voltage = std::abs(simResult.nodeVoltage(driverSource._posNode, simResult.size()-1));
      double newEffCap = (totalCharge - _capCharge) / (voltage - _voltage);
      _capCharge = totalCharge;
      _voltage = voltage;
      _effCap = newEffCap;
    }
  }

}

void
CSMDriver::updateCircuit(const SimResult& simResult) const
{
  /// First check current simTime, and choose the correct effCap from _effCaps, update circuit
  double effCap = calcEffectiveCap(simResult, timeStart, timeEnd);
  /// append voltage waveform from timeStart to timeEnd, to _driverArc->driverSource
  
}



}