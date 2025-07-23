#include "CSMDriver.h"

namespace NA {

void 
CSMDriver::init(Circuit* ckt, const CellArc* driverArc, bool isRise)
{
  _driverArc = driverArc;
  _ckt = ckt;
  _driverData.init(driverArc->ccsData(), isRise);
}

double
CSMDriver::calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const
{
  if (simResult.empty()) {
    double totalCap = 0;
    size_t vsrcId = _driverArc->driverSourceId();
    const std::vector<const Device*>& connDevs = _ckt->traceDevice(vsrcId);
    for (const Device* dev : connDevs) {
      if (dev->_type == DeviceType::Capacitor) {
        totalCap += dev->_value;
      }
    }
    return totalCap;
  } else {
    if (simResult.currentTime() < timeEnd) {
      return _effCap;
    } else {
      const Device& driverSource = _ckt->device(_driverArc->driverSourceId);
      double totalCharge = std::abs(simResult.totalCharge(driverSource));
      double voltage = std::abs(simResult.nodeVoltage(driverSource._posNode, simResult.size()-1));
      double newEffCap = (totalCharge - _capCharge) / (voltage - _voltage);
      //_capCharge = totalCharge;
      //_voltage = voltage;
    }
  }

}

void
CSMDriver::updateCircuit(const SimResult& simResult, double timeStart, double timeEnd) const
{
  double effCap = calcEffectiveCap(simResult, timeStart, timeEnd);
  
}



}