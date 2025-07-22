#include "CSMDriver.h"

namespace NA {

void 
CSMDriver::init(Circuit* ckt, const CellArc* driverArc, bool isRise)
{
  _driverArc = driverArc;
  _ckt = ckt;
  _driverData.init(driverArc->ccsData(), isRise);
}





}