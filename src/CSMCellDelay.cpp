#include "CSMCellDelay.h"

namespace NA {

CSMCellDelay(const CellArc* cellArc, Circuit* ckt, bool isMaxDelay)
: _cellArc(cellArc), _ckt(ckt), 
  _libData(cellArc->nldmData()->owner()), 
  _isMaxDelay(isMaxDelay)
{
  size_t rdId = driverArc->driverResistorId();
  const std::vector<const Device*>& connDevs = ckt->traceDevice(rdId);
  std::vector<const CellArc*> retval;
  for (const Device* dev : connDevs) {
    if (dev->_type == DeviceType::Capacitor && dev->_isInternal) {
      _loadCaps.push_back(dev);
    }
  }
}

void 
CSMCellDelay::initData()
{
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
}

void
CSMCellDelay::updateCircuit() const
{
  if (_simResult.empty() == false) {
    updateReceiverCap();
  } else {
    initReceiverCap();
  }
  _driver.updateCircuit(_simResult);
}

void
CSMCellDelay::updateReceiverCap() const
{
  for (Device* loadCap : _loadCaps) {
    double cap = _isMaxDelay ? 0 : 1e99;
    const std::vector<CellArc*>& loadArcs = ckt->cellArcsOfDevice(loadCap);
    assert(arcs.empty() == false);
    for (const CellArc* loadArc : loadArcs) {
      CSMReceiver recvr(_ckt, loadArc);
      double loadCap = recvr.calcLoadCap();
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
CSMCellDelay::initReceiverCap() const
{
  for (Device* loadCap : _loadCaps) {
    double cap = _isMaxDelay ? 0 : 1e99;
    const std::vector<CellArc*>& arcs = _ckt->cellArcsOfDevice(dev);
    assert(arcs.empty() == false);
    for (const CellArc* loadArc : arcs) {
      if (_isMaxDelay) {
        cap = std::max(cap, loadArc->fixedLoadCap(_isRiseOnDriverPin));
      } else {
        cap = std::min(cap, loadArc->fixedLoadCap(_isRiseOnDriverPin));
      }
    }
    loadCap->_value = cap;
    if (Debug::enabled(DebugModule::CCS)) {
      printf("DEBUG: Load cap %s init value set to %G\n", loadCap->_name.data(), loadCap->_value);
    }
  }
}


}