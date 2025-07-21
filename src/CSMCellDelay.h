#ifndef _NA_CSMCELLDLY_H_
#define _NA_CSMCELLDLY_H_

#include "Circuit.h"
#include "LibData.h"
#include "SimResult.h"

namespace NA {

class CellArc;
class Circuit;

class CSMCellDelay {
  public: 
    CSMCellDelay(const CellArc* cellArc, Circuit* ckt)
    : _cellArc(cellArc), _ckt(ckt), _libData(cellArc->nldmData()->owner()) {}

    bool calculate();

    SimResult result() const { return _finalResult; }
    bool isRiseOnOutputPin() const { return _isRiseOnDriverPin; }

  private:

  private:
    const CellArc* _cellArc;
    Circuit* _ckt;
    const LibData* _libData;
    SimResult _finalResult;
    bool   _isRiseOnInputPin = true;
    bool   _isRiseOnDriverPin = true;
    bool   _setTerminationCondition = false;
    double _delayThres = 50;
    double _tranThres1 = 10;
    double _tranThres2 = 90;
};

}




}

#endif
