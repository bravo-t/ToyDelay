#include <cstdio>
#include <cstring>
#include "DelayCalculator.h"
#include "Base.h"
#include "NetlistParser.h"
#include "RampVDelay.h"
#include "CSMDelay.h"
#include "Timer.h"
#include "StringUtil.h"

namespace NA {

void
DelayCalculator::run(const char* inFile) 
{
  NetlistParser parser(inFile);
  const std::vector<AnalysisParameter>& params = parser.analysisParameters();
  for (const AnalysisParameter& param : params) {
    if (param._type == NA::AnalysisType::FD) {
      if (param._driverModel == NA::DriverModel::RampVoltage) {
        RampVDelay delayCalc(param, parser);
        delayCalc.calculate();
      }
      if (param._driverModel == NA::DriverModel::PWLCurrent) {
        CSMDelay delayCalcMax(param, parser, true);
        delayCalcMax.calculate();
        CSMDelay delayCalcMin(param, parser, false);
        delayCalcMin.calculate();
      }
    }
  }
}

}
