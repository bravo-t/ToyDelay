#ifndef _NA_CCSRECV_H_
#define _NA_CCSRECV_H_

#include <vector>
#include <cassert>
#include "Base.h"
#include "LibData.h"

namespace NA {

class Circuit;
class CellArc;
class SimResult;
   
class CSMReceiver {
  public:
    CSMReceiver(Circuit* ckt, const CellArc* loadArc);
    /// This function is called inside CSM calculation iteration
    /// to update receiver capacitors
    double capValue(const SimResult& simResult) const;
    /// This function is called after a CSM calculation iteration is finished
    /// to calculate receiver cap values
    void calcReceiverCap(const SimResult& simResult);

    const CellArc* loadArc() const { return _loadArc; }

  private:
  private:
    bool                _isInputRise = true;
    const CellArc*      _loadArc = nullptr;
    Circuit*            _ckt = nullptr;
    std::vector<double> _recvCaps;
    std::vector<double> _capThresholdVoltage;
};

}

#endif