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
  double dv = voltage / recvCapLUT.size();
  for (size_t i=0; i<recvCapLUT.size(); ++i) {
    _capThresholdVoltage.push_back(i*dv);
    const NLDMLUT& lut = recvCapLUT[i];
    _recvCaps.push_back(lut.value(inputTran, effCap));
  }
}

void 
CSMReceiver::updateCircuit(const SimResult& simResult)
{
  double loadCap = 0;
  if (simResult.empty() == false) {
    const Waveform& loadPinWaveform = simResult.nodeVoltage(_loadArc->inputTranNode());

  }
}

}