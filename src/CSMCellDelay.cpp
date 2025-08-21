#include "CSMCellDelay.h"

namespace NA {

CSMCellDelay(const CellArc* cellArc, Circuit* ckt, bool isMaxDelay)
: _cellArc(cellArc), _ckt(ckt), 
  _libData(cellArc->nldmData()->owner()), 
  _isMaxDelay(isMaxDelay)
{}


void
markSimulationScope(size_t devId, Circuit* ckt)
{
  const std::vector<const Device*>& connDevs = ckt->traceDevice(devId);
  ckt->resetSimulationScope();
  ckt->markSimulationScope(connDevs);
}

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
  size_t rdId = _cellArc->driverResistorId();
  const std::vector<const Device*>& connDevs = _ckt->traceDevice(rdId);
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor && dev->_isInternal) {
      _loadCaps.push_back(dev);
      const std::vector<CellArc*>& loadArcs = _ckt->cellArcsOfDevice(dev);
      ReceiverVec recvr;
      for (const CellArc* loadArc : loadArcs) {
        recvr.push_back(CSMReceiver(_ckt, loadArc));
      }
      _receiverMap.insert({dev._devId, recvr});
    }
  }
  markSimulationScope(vSrcId, _ckt);
}

bool
CSMCellDelay::updateCircuit() const
{
  updateReceiverCap(_simResult);
  return _driver.updateCircuit(_simResult);
}

void
CSMCellDelay::updateReceiverCap(const SimResult& simResult) const
{
  for (Device* loadCap : _loadCaps) {
    size_t capId = loadCap->_devId;
    const auto& found = _receiverMap.find(capId);
    assert(found != _receiverMap.end());
    const ReceiverVec& rcvModels = found->second;
    double cap = _isMaxDelay ? 0 : 1e99;
    for (const CSMReceiver& rcvModel : rcvModels) {
      double capValue = rcvModel.capValue(simResult);
      if (_isMaxDelay) {
        cap = std::max(cap, loadCap);
      } else {
        cap = std::min(cap, loadCap);
      }
    }
    loadCap->_value = cap;
    if (Debug::enabled(DebugModule::CCS)) {
      printf("DEBUG: Load cap %s value updated to %G\n", loadCap->_name.data(), loadCap->_value);
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
  converged = updateCircuit(_simResult);
  _simResult.clear();
  AnalysisParameter simParam;
  simParam._type = AnalysisType::Tran;
  simParam._simTime = _tDelta * 1.2;
  simParam._simTick = simParam._simTime / 1000;
  simParam._intMethod = IntegrateMethod::Trapezoidal;
  Simulator sim(*_ckt, simParam);
  std::function<void(void)> f = [this, &simResult]() {
    this->updateReceiverCap(simResult);
  };
  sim.setUpdateFunction(f);
  if (Debug::enabled(DebugModule::NLDM)) {
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
}

}