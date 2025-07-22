#include "CSMCellDelay.h"

namespace NA {

CSMCellDelay(const CellArc* cellArc, Circuit* ckt)
: _cellArc(cellArc), _ckt(ckt), _libData(cellArc->nldmData()->owner()) 
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




}