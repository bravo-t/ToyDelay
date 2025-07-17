#include <vector>
#include "Base.h"
#include "LibData.h"

namespace NA {

class CCSDriverData {
  public:
    CCSDriverData(const CCSArc* cellArc, bool isRise);
    
    double referenceTime(double inputTran) const;
    Waveform driverWaveform(double inputTran, double outputLoad);

  private:
    initVoltageWaveforms(const CCSGroup& luts);
    const CCSGroup& ccsGroup() const;
    const CCSLUT& ccsTable(size_t index) const;
    Waveform interpolateVoltageWaveforms(double inputTran, double outputLoad, 
                                         const std::vector<double>& timeSteps) const;


  private:
    bool                  _isRise;
    const CCSArc*         _arc;
    std::vector<Waveform> _voltageWaveforms;

};




}