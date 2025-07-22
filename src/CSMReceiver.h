#ifndef _NA_CCSRECV_H_
#define _NA_CCSRECV_H_

#include <vector>
#include "Base.h"
#include "LibData.h"

namespace NA {

class Circuit;
   
class CSMReceiver {
  public:
    CSMReceiver(Circuit* ckt, const CellArc* loadArc);
    void calcLoadCap() const;

  private:
  private:
    const CellArc* _loadArc = nullptr;
    Circuit*       _ckt = nullptr;
};

}

#endif