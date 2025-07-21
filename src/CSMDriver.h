#ifndef _NA_CSMDRVR_H_
#define _NA_CSMDRVR_H_

#include "CCSDriverData.h"

namespace NA {

class Circuit;
class CellArc;

class CSMDriver {
  public:
    CSMDriver(Circuit* ckt, const CellArc* driverArc);
    updateDriver() const;
  private:
    CCSDriverData _driverData;
}




}

#endif