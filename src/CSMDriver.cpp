#include "CSMDriver.h"

namespace NA {

void 
CSMDriver::init(Circuit* ckt, const CellArc* driverArc, bool isRise)
{
  _driverArc = driverArc;
  _ckt = ckt;
  _driverData.init(driverArc->ccsData(), isRise);
  _inputTran = _driverArc->inputTransition(_ckt);
}

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
  double effCap = calcEffectiveCap(simResult, timeStart, timeEnd);
  /// append voltage waveform from timeStart to timeEnd, to _driverArc->driverSource
  
}



}