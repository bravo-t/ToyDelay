#include "LibData.h"
#include "CSMReceiver.h"
#include "RampVCellDelay.h"

namespace NA {

CSMReceiver::CSMReceiver(Circuit* ckt, const CellArc* loadArc)
: _loadArc(loadArc), _ckt(ckt) {}

void
CSMReceiver::calcReceiverCap(const SimResult& SimResult) 
{
  const Waveform& loadPinWaveform = simResult.nodeVoltageWaveform(_loadArc->inputTranNode());
  bool isRise = loadPinWaveform.isRise();
  _isInputRise = isRise;
  const LibData* libData = _loadArc->libData();
  double inputTran = loadPinWaveform.transitionTime(libata);
  RampVCellDelay nldmCalc(_loadArc, _ckt);
  nldmCalc.setInputTransition(inputTran);
  nldmCalc.setIsInputTranRise(isRise);
  bool success = nldmCalc.calculate();
  double effCap = nldmCalc.effCap();
  LUTType rcvCapLUTType = LUTType::RiseRecvCap;
  if (isRise == false) {
    rcvCapLUTType = LUTType::FallRecvCap;
  }
  const std::vector<NLDMLUT>& recvCapLUT = _loadArc->getRecvCap(rcvCapLUTType);
  double voltage = libData->voltage();
  if (isRise == false) voltage = -voltage;
  double dv = voltage / recvCapLUT.size();
  for (size_t i=0; i<recvCapLUT.size(); ++i) {
    _capThresholdVoltage.push_back(i*dv);
    const NLDMLUT& lut = recvCapLUT[i];
    _recvCaps.push_back(lut.value(inputTran, effCap));
  }
  _capThresholdVoltage.push_back(voltage);
}

double
CSMReceiver::capValue(const SimResult& simResult) const
{
  double loadCap = 0;
  if (simResult.empty() == false) {
    double inputVoltage = simResult.latestVoltage(_loadArc->inputTranNode());
    for (size_t i=1; i<_capThresholdVoltage.size(); ++i) {
      double thresVoltage2 = _capThresholdVoltage[i];
      double thresVoltage1 = _capThresholdVoltage[i-1];
      if (_isInputRise) {
        if (inputVoltage > thresVoltage1 && inputVoltage <= thresVoltage2) {
          loadCap = _recvCaps[i-1];
          break;
        }
      } else {
        if (inputVoltage < thresVoltage1 && inputVoltage >= thresVoltage2) {
          loadCap = _recvCaps[i-1];
          break;
        }
      }
    }
  } else {
    loadCap = _loadArc->fixedLoadCap(_isInputRise);
  }
  return loadCap;
}

}