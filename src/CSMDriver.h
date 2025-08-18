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
    void init(Circuit* ckt, const CellArc* driverArc, bool isRise, const SimResult& simResult);
    /// timeStart and timeEnd are the start and end time of current voltage region
    /// UPDATE: remove timeStart and timeEnd, generate full driver voltage waveform instead, 
    ///         If simResult is empty, effCap is constant throughout the simulation, 
    ///         If simResult is available, generate effCaps for each voltage region based on simResult
    ///         Then updateCircuit to set the simulation data
    ///          
    //void updateCircuit(const SimResult& simResult, double timeStart, double timeEnd) const;
    void updateCircuit(const SimResult& simResult) const;

  private:
    void calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const;
  private:
    const CellArc* _driverArc = nullptr;
    Circuit*       _ckt = nullptr;
    double         _inputTran = 0;
    std::vector<double> _timeSteps;
    std::vector<double> _effCaps;
    CCSDriverData  _driverData;
}




}

#endif