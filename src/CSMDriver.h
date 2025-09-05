#ifndef _NA_CSMDRVR_H_
#define _NA_CSMDRVR_H_

#include <vector>
#include "Base.h"
#include "LibData.h"
#include "SimResult.h"

namespace NA {

class Circuit;
class CellArc;

class CSMDriverData {
  public:
    CSMDriverData() = default;
    void init(const CCSArc* cellArc, bool isRise);
    
    double referenceTime(double inputTran) const;
    Waveform driverWaveform(double inputTran, double outputLoad) const;
    double timeAtVoltage(double inputTran, double outputLoad, double voltage) const;
    std::vector<double> timeSteps(double inputTran, const std::vector<double>& effCaps) const;
    std::vector<double> timeSteps(double inputTran, double outputLoad) const;
    std::vector<double> voltageRegions() const { return _voltageSteps; }

    double simTerminalVoltage() const { return _termVoltage; }

  private:
    void initVoltageWaveforms(const CCSGroup& luts);
    const CCSGroup& ccsGroup() const;
    const CCSLUT& ccsTable(size_t index) const;
    Waveform interpolateVoltageWaveforms(double inputTran, double outputLoad, 
                                         const std::vector<double>& timeSteps) const;

  private:
    bool                  _isRise;
    const CCSArc*         _arc;
    double                _termVoltage;
    double                _vth;
    double                _vl;
    double                _vh;
    std::vector<Waveform> _voltageWaveforms;
    std::vector<double>   _voltageSteps;
};

class CSMDriver {
  public:
    CSMDriver() = default;
    void init(Circuit* ckt, const CellArc* driverArc, bool isRise, bool isMax);
    /// Generate full driver voltage waveform.
    /// If simResult is empty, effCap is constant throughout the simulation, 
    /// If simResult is available, generate effCaps for each voltage region based on simResult
    /// Then updateCircuit to set the simulation data
    /// The bool return value indicates if there is no significant change in _effCaps,
    /// which can be used to tell if the calculation is converged.
    bool updateCircuit(const SimResult& simResult);
    double inputTransition() const { return _inputTran; }
    double simTerminalVoltage() const { return _driverData.simTerminalVoltage(); }
    double inputReferenceTime() const { return _driverData.referenceTime(_inputTran); }

  private:
    double calcEffectiveCap(const SimResult& simResult, double timeStart, double timeEnd) const;
    bool updateDriverData(const SimResult& simResult);
    void updateTimeSteps(const SimResult& simResult);

  private:
    bool           _isMax = true;
    bool           _isRise = true;
    const CellArc* _driverArc = nullptr;
    Circuit*       _ckt = nullptr;
    double         _inputTran = 0;
    std::vector<double> _timeSteps;
    std::vector<double> _effCaps;
    CSMDriverData  _driverData;
};

}

#endif