#ifndef _NA_CSMCELLDLY_H_
#define _NA_CSMCELLDLY_H_

#include "Circuit.h"
#include "LibData.h"
#include "SimResult.h"
#include "CSMDriver.h"
#include "CSMReceiver.h"

namespace NA {

class CellArc;
class Circuit;

class CSMCellDelay {
  public: 
    CSMCellDelay(const CellArc* cellArc, Circuit* ckt, bool isMaxDelay);

    bool calculate();

    SimResult result() const { return _simResult; }
    bool isRiseOnOutputPin() const { return _isRiseOnDriverPin; }

    std::vector<const CellArc*> loadArcs() const;

  private:
    bool updateCircuit();
    void initData();
    bool updateReceiverCap(const SimResult& simResult) const;
    void updateReceiverModel(const SimResult& simResult) const;
    bool calcIteration(bool& converged);


  private:
    const CellArc*       _cellArc;
    Circuit*             _ckt;
    const LibData*       _libData;
    SimResult            _simResult;
    bool                 _isRiseOnInputPin = true;
    bool                 _isRiseOnDriverPin = true;
    bool                 _setTerminationCondition = false;
    bool                 _isMaxDelay = true;
    double               _delayThres = 50;
    double               _tranThres1 = 10;
    double               _tranThres2 = 90;
    CSMDriver            _driver;   
    
    typedef std::vector<CSMReceiver> ReceiverVec;
    typedef std::unordered_map<size_t, ReceiverVec> ReceiverMap;
    ReceiverMap          _receiverMap;
    std::vector<size_t>  _loadCaps;
};

}

#endif
