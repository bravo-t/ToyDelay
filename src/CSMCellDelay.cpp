#include "CSMCellDelay.h"
#include "CommonUtils.h"
#include "Simulator.h"
#include "Debug.h"
#include "Plotter.h"

namespace NA {

CSMCellDelay::CSMCellDelay(const CellArc* cellArc, Circuit* ckt, bool isMaxDelay)
: _cellArc(cellArc), _ckt(ckt), 
  _libData(cellArc->nldmData()->owner()), 
  _isMaxDelay(isMaxDelay)
{
  initData();
}

static size_t invalidId = static_cast<size_t>(-1);

void 
CSMCellDelay::initData()
{
  /// init driver
  size_t vSrcId = _cellArc->inputSourceDevId(_ckt);
  if (vSrcId == invalidId) {
    printf("Cannot find input source device on driver model\n");
    return;
  }
  const Device& vSrc = _ckt->device(vSrcId);
  const PWLValue& data = _ckt->PWLData(vSrc);

  _isRiseOnInputPin = data.isRiseTransition();

  _isRiseOnDriverPin = (_isRiseOnInputPin != _cellArc->isInvertedArc());

  _driver.init(_ckt, _cellArc, _isRiseOnDriverPin, _isMaxDelay);
  
  /// init receiver
  size_t drvId = _cellArc->driverSourceId();
  const std::vector<const Device*>& connDevs = _ckt->traceDevice(drvId);
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor && dev->_isInternal) {
      _loadCaps.push_back(dev->_devId);
      const std::vector<CellArc*>& loadArcs = _ckt->cellArcsOfDevice(dev);
      ReceiverVec recvr;
      for (const CellArc* loadArc : loadArcs) {
        recvr.push_back(CSMReceiver(_ckt, loadArc, _isRiseOnDriverPin));
      }
      _receiverMap.insert({dev->_devId, recvr});
    }
  }
  markSimulationScope(drvId, _ckt);
}

bool
CSMCellDelay::updateCircuit()
{
  updateReceiverModel(_simResult);
  return _driver.updateCircuit(_simResult);
}

bool
CSMCellDelay::updateReceiverCap(const SimResult& simResult) const
{
  bool valuesUpdated = false;
  for (size_t capId : _loadCaps) {
    const auto& found = _receiverMap.find(capId);
    assert(found != _receiverMap.end());
    const ReceiverVec& rcvModels = found->second;
    double cap = _isMaxDelay ? 0 : 1e99;
    for (const CSMReceiver& rcvModel : rcvModels) {
      double capValue = rcvModel.capValue(simResult);
      if (_isMaxDelay) {
        cap = std::max(cap, capValue);
      } else {
        cap = std::min(cap, capValue);
      }
    }
    Device& capDev = _ckt->device(capId);
    if (capDev._value != cap) {
      capDev._value = cap;
      if (Debug::enabled(DebugModule::CCS)) {
        printf("DEBUG: T@%G Load cap %s value updated to %G\n", simResult.currentTime(), capDev._name.data(), capDev._value);
      }
      valuesUpdated = true;
    }
  }
  return valuesUpdated;
}

void
CSMCellDelay::updateReceiverModel(const SimResult& simResult)
{
  for (auto& kv : _receiverMap) {
    ReceiverVec& rcvModels = kv.second;
    for (CSMReceiver& rcvModel : rcvModels) {
      rcvModel.calcReceiverCap(simResult);
    }
  }
}

bool
CSMCellDelay::calcIteration(bool& converged)
{
  ++_iterCount;
  if (Debug::enabled(DebugModule::CCS) && _simResult.empty() == false) {
    PlotData cellArcPlotData;
    cellArcPlotData._canvasName = "Intermediate calculate result for iteration ";
    cellArcPlotData._canvasName += std::to_string(_iterCount);
    populatePlotData(cellArcPlotData, _cellArc->inputNode(), _cellArc->outputNode(_ckt), _ckt);
    Plotter::plot(cellArcPlotData, {*_ckt}, {_simResult});

    cellArcPlotData._nodeToPlot.clear();
    cellArcPlotData._nodeSimName.clear();
    for (size_t loadCapId : _loadCaps) {
      const Device& loadCap = _ckt->device(loadCapId);
      cellArcPlotData._nodeToPlot.push_back(_ckt->node(loadCap._posNode)._name);
      cellArcPlotData._nodeSimName.push_back("fd");
    }
    Plotter::plot(cellArcPlotData, {*_ckt}, {_simResult});
  }

  converged = updateCircuit();
  _simResult.clear();
  AnalysisParameter simParam;
  simParam._name = "fd";
  simParam._type = AnalysisType::Tran;
  simParam._simTime = _driver.inputTransition() * 100;
  simParam._simTick = _driver.inputTransition() / 100;
  simParam._intMethod = IntegrateMethod::BackwardEuler;
  Simulator sim(*_ckt, simParam);
  setTerminationCondition(_ckt, _cellArc, _isRiseOnDriverPin, sim);
  std::function<bool(void)> f = [this, &sim]() {
    return this->updateReceiverCap(sim.simulationResult());
  };
  sim.setUpdateFunction(f);
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: start transient simualtion for CCS calculation\n");
  }
  sim.run();
  _simResult = sim.simulationResult();
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: Simulation finished in T@%G, expected %G\n", _simResult.currentTime(), simParam._simTime);
  }
  return true;
}

bool
CSMCellDelay::calculate()
{
  bool converged = false;
  while (!converged) {
    calcIteration(converged);
  }
  return converged;
}

std::vector<const CellArc*>
CSMCellDelay::loadArcs() const
{
  std::vector<const CellArc*> arcs;
  for (const auto& kv : _receiverMap) {
    const ReceiverVec& rcvrs = kv.second;
    for (const CSMReceiver& rcvr : rcvrs) {
      arcs.push_back(rcvr.loadArc());
    }
  }
  return arcs;
}

}