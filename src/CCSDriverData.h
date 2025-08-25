#ifndef _NA_CCSDRV_H_
#define _NA_CCSDRV_H_

#include <vector>
#include "Base.h"
#include "LibData.h"

namespace NA {

class CCSDriverData {
  public:
    CCSDriverData() = default;
    void init(const CCSArc* cellArc, bool isRise);
    
    double referenceTime(double inputTran) const;
    Waveform driverWaveform(double inputTran, double outputLoad);
    double timeAtVoltage(double inputTran, double outputLoad, double voltage) const;
    std::vector<double> timeSteps(double inputTran, const std::vector<double>& effCaps) const;
    std::vector<double> timeSteps(double inputTran, double outputLoad) const;
    std::vector<double> voltageRegions() const { return _voltageSteps; }


  private:
    void initVoltageWaveforms(const CCSGroup& luts);
    const CCSGroup& ccsGroup() const;
    const CCSLUT& ccsTable(size_t index) const;
    Waveform interpolateVoltageWaveforms(double inputTran, double outputLoad, 
                                         const std::vector<double>& timeSteps) const;

  private:
    bool                  _isRise;
    const CCSArc*         _arc;
    double                _vth;
    double                _vl;
    double                _vh;
    std::vector<Waveform> _voltageWaveforms;
    std::vector<double>   _voltageSteps;
};

}

#endif