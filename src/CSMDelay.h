#ifndef _NA_CSMDLY_H_
#define _NA_CSMDLY_H_

#include <tuple>
#include <vector>
#include "Base.h"
#include "NetlistParser.h"
#include "Circuit.h"

namespace NA {

class CSMDelay {
  public:
    CSMDelay(const AnalysisParameter& param, const NetlistParser& parser, bool isMaxDelay);

    void calculate();

  private:
    void calculateArc(const CellArc* driverArc);

  private:
    bool    _isMaxDelay;
    Circuit _ckt;
    std::vector<const CellArc*> _cellArcs;
};




}

#endif