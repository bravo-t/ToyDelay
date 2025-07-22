#ifndef _NA_CSMDRVR_H_
#define _NA_CSMDRVR_H_

#include "CCSDriverData.h"
#include "SimResult.h"

namespace NA {

class Circuit;
class CellArc;

class CSMDriver {
  public:
    CSMDriver() = default;
    void init(Circuit* ckt, const CellArc* driverArc, bool isRise);
    void updateCircuit(const SimResult& simResult) const;
  private:
    const CellArc* _driverArc = nullptr;
    Circuit*       _ckt = nullptr;
    CCSDriverData  _driverData;
}




}

#endif