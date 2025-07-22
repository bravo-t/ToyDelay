#ifndef _NA_CSMCELLDLY_H_
#define _NA_CSMCELLDLY_H_

#include "Circuit.h"
#include "LibData.h"
#include "SimResult.h"
#include "CSMDriver.h"

namespace NA {

class CellArc;
class Circuit;

class CSMCellDelay {
  public: 
    CSMCellDelay(const CellArc* cellArc, Circuit* ckt);

    bool calculate();

    SimResult result() const { return _finalResult; }
    bool isRiseOnOutputPin() const { return _isRiseOnDriverPin; }

  private:
    void updateCircuit() const;
    void initData();

  private:
    const CellArc* _cellArc;
    Circuit* _ckt;
    const LibData* _libData;
    std::vector<Device*> _loadCaps;
    SimResult _finalResult;
    bool   _isRiseOnInputPin = true;
    bool   _isRiseOnDriverPin = true;
    bool   _setTerminationCondition = false;
    double _delayThres = 50;
    double _tranThres1 = 10;
    double _tranThres2 = 90;
    CSMDriver _driver;   
};

}




}

#endif
