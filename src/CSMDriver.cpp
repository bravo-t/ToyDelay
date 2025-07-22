#include "CSMDriver.h"

namespace NA {

void 
CSMDriver::init(Circuit* ckt, const CellArc* driverArc, bool isRise)
{
  _driverArc = driverArc;
  _ckt = ckt;
  _driverData.init(driverArc->ccsData(), isRise);
}

void
CSMDriver::updateCircuit(const SimResult& simResult) const
{
  double effCap = 0;
  if (simResult.empty()) {
    double totalCap = 0;
    size_t vsrcId = _driverArc->driverSourceId();
    const std::vector<const Device*>& connDevs = _ckt->traceDevice(vsrcId);
    for (const Device* dev : connDevs) {
      if (dev->_type == DeviceType::Capacitor) {
        totalCap += dev->_value;
      }
    }
    effCap = totalCap;
  } else {

  }
}



}