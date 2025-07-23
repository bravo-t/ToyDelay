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
    /// timeStart and timeEnd are the start and end time of current voltage region
    void updateCircuit(const SimResult& simResult, double timeStart, double timeEnd) const;

    void cacheData(const SimResult& simResult, double timeStart, double timeEnd);
  private:
    void calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const;
  private:
    const CellArc* _driverArc = nullptr;
    Circuit*       _ckt = nullptr;
    double         _effCap = 0; /// Cache for performance
    double         _capCharge = 0; /// charge value for previous iteration of simulation
    double         _voltage = 0;   /// voltage of previous iteration
    CCSDriverData  _driverData;
}




}

#endif