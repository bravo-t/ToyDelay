#ifndef _NA_DLY_COMUTL_H_
#define _NA_DLY_COMUTL_H_

#include "CommonUtils.h"
#include "Circuit.h"
#include "SimResult.h"
#include "Simulator.h"
#include "LibData.h"
#include "Plotter.h"

namespace NA {

inline std::vector<const CellArc*>
setTerminationCondition(const Circuit* ckt, const CellArc* driverArc, 
                        bool isRiseOnDriverPin, Simulator& sim)
{
  size_t drvId = driverArc->driverSourceId();
  const std::vector<const Device*>& connDevs = ckt->traceDevice(drvId);
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

inline void
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

inline void
markSimulationScope(size_t devId, Circuit* ckt)
{
  const std::vector<const Device*>& connDevs = ckt->traceDevice(devId);
  ckt->resetSimulationScope();
  ckt->markSimulationScope(connDevs);
}

inline void 
populatePlotData(PlotData& plotData, size_t fromNodeId, size_t toNodeId, const Circuit* ckt)
{
  const char* simName = "fd";
  plotData._nodeToPlot.push_back(ckt->node(fromNodeId)._name);
  plotData._nodeSimName.push_back(simName);
  plotData._nodeToPlot.push_back(ckt->node(toNodeId)._name);
  plotData._nodeSimName.push_back(simName);
}

}

#endif