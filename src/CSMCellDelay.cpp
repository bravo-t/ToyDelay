#include "CSMCellDelay.h"
#include "CommonUtils.h"
#include "Simulator.h"
#include "Debug.h"

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

  _driver.init(_ckt, _cellArc, _isRiseOnDriverPin);
  
  /// init receiver
  size_t drvId = _cellArc->driverSourceId();
  const std::vector<const Device*>& connDevs = _ckt->traceDevice(drvId);
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor && dev->_isInternal) {
      _loadCaps.push_back(dev->_devId);
      const std::vector<CellArc*>& loadArcs = _ckt->cellArcsOfDevice(dev);
      ReceiverVec recvr;
      for (const CellArc* loadArc : loadArcs) {
        recvr.push_back(CSMReceiver(_ckt, loadArc));
      }
      _receiverMap.insert({dev->_devId, recvr});
    }
  }
  markSimulationScope(drvId, _ckt);
}

bool
CSMCellDelay::updateCircuit()
{
  updateReceiverCap(_simResult);
  return _driver.updateCircuit(_simResult);
}

void
CSMCellDelay::updateReceiverCap(const SimResult& simResult) const
{
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
    capDev._value = cap;
    if (Debug::enabled(DebugModule::CCS)) {
      printf("DEBUG: Load cap %s value updated to %G\n", capDev._name.data(), capDev._value);
    }
  }
}

void
CSMCellDelay::updateReceiverModel(const SimResult& simResult) const
{
  for (auto kv : _receiverMap) {
    ReceiverVec& rcvModels = kv.second;
    for (CSMReceiver& rcvModel : rcvModels) {
      rcvModel.calcReceiverCap(simResult);
    }
  }
}

bool
CSMCellDelay::calcIteration(bool& converged)
{
  converged = updateCircuit();
  _simResult.clear();
  AnalysisParameter simParam;
  simParam._type = AnalysisType::Tran;
  simParam._simTime = _driver.inputTransition() * 10;
  simParam._simTick = _driver.inputTransition() / 100;
  simParam._intMethod = IntegrateMethod::Trapezoidal;
  Simulator sim(*_ckt, simParam);
  setTerminationCondition(_ckt, _cellArc, _isRiseOnDriverPin, sim);
  std::function<void(void)> f = [this]() {
    this->updateReceiverCap(this->_simResult);
  };
  sim.setUpdateFunction(f);
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: start transient simualtion\n");
  }
  sim.run();
  _simResult = sim.simulationResult();
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