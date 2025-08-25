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
    /// Generate full driver voltage waveform.
    /// If simResult is empty, effCap is constant throughout the simulation, 
    /// If simResult is available, generate effCaps for each voltage region based on simResult
    /// Then updateCircuit to set the simulation data
    /// The bool return value indicates if there is no significant change in _effCaps,
    /// which can be used to tell if the calculation is converged.
    bool updateCircuit(const SimResult& simResult);
    double inputTransition() const { return _inputTran; }

  private:
    double calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const;
    bool updateDriverData(const SimResult& simResult);

  private:
    const CellArc* _driverArc = nullptr;
    Circuit*       _ckt = nullptr;
    double         _inputTran = 0;
    std::vector<double> _timeSteps;
    std::vector<double> _effCaps;
    CCSDriverData  _driverData;
};




}

#endif