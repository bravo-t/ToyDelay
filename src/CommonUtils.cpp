#include "CommonUtils.h"
#include "Circuit.h"
#include "SimResult.h"
#include "Simulator.h"
#include "LibData.h"

namespace NA {

std::vector<const CellArc*>
setTerminationCondition(const Circuit* ckt, const CellArc* driverArc, 
                        bool isRiseOnDriverPin, Simulator& sim)
{
  size_t rdId = driverArc->driverResistorId();
  const std::vector<const Device*>& connDevs = ckt->traceDevice(rdId);
  std::vector<const CellArc*> retval;
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor && dev->_isInternal) {
      const std::vector<CellArc*>& loadArcs = ckt->cellArcsOfDevice(dev);
      double termVoltage = 0;
      const CellArc* loadArc = nullptr;
      for (const CellArc* cellArc : loadArcs) {
        const LibData* libData = cellArc->libData();
        double libVoltage = libData->voltage();
        double termPoint = libData->riseTransitionHighThres();
        if (isRiseOnDriverPin == false) {
          termPoint = libData->fallTransitionLowThres() - 100;
        }
        double v = libVoltage * termPoint / 100;
        if (isRiseOnDriverPin) {
          if (v > termVoltage) {
            termVoltage = v;
            loadArc = cellArc;
          }
        } else {
          if (v < termVoltage) {
            termVoltage = v;
            loadArc = cellArc;
          }
        }
      }
      retval.push_back(loadArc);
      const Node& posNode = ckt->node(dev->_posNode);
      const Node& negNode = ckt->node(dev->_negNode);
      assert(posNode._isGround != negNode._isGround);
      if (posNode._isGround) {
        sim.setTerminationVoltage(negNode._nodeId, termVoltage);
      } else {
        sim.setTerminationVoltage(posNode._nodeId, termVoltage);
      }
    }
  }
  return retval;
}

void
measureVoltage(const SimResult& result, size_t nodeId, const LibData* libData,  
               double& delay, double& trans)
{
  const Waveform& nodeVoltage = result.nodeVoltageWaveform(nodeId);
  bool isRise = nodeVoltage.isRise();
  double delayThres = libData->riseDelayThres();
  double lowerThres = libData->riseTransitionLowThres();
  double upperThres = libData->riseTransitionHighThres();
  if (isRise == false) {
    delayThres = -libData->fallDelayThres();
    lowerThres = libData->fallTransitionLowThres();
    upperThres = libData->fallTransitionHighThres();
  }
  double libVoltage = libData->voltage();
   
  delay = nodeVoltage.measure(delayThres / 100 * libVoltage);
  if (nodeVoltage.isRise()) {
    double transLower = nodeVoltage.measure(lowerThres / 100 * libVoltage);
    double transUpper = nodeVoltage.measure(upperThres / 100 * libVoltage);
    trans = transUpper - transLower;
  } else {
    double upperVoltage = (upperThres - 100) / 100 * libVoltage;
    double lowerVoltage = (lowerThres - 100) / 100 * libVoltage;
    double transLower = nodeVoltage.measure(lowerVoltage);
    double transUpper = nodeVoltage.measure(upperVoltage);
    if (transLower == 1e99 && transUpper == 1e99 && upperVoltage < 0 && lowerVoltage < 0 && libVoltage > 0) {
      upperVoltage = libVoltage + upperVoltage;
      lowerVoltage = libVoltage + lowerVoltage;
      transLower = nodeVoltage.measure(lowerVoltage);
      transUpper = nodeVoltage.measure(upperVoltage);
    }
    trans = transLower - transUpper;
  }
}

}