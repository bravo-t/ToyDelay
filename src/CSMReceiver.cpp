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
  double inputTran = loadPinWaveform.transitionTime(_loadArc->libData());
  RampVCellDelay nldmCalc(_loadArc, _ckt);
  nldmCalc.setInputTransition(inputTran);
  nldmCalc.setIsInputTranRise(isRise);
  bool success = nldmCalc.calculate();
  double effCap = nldmCalc.effCap();
  
}


}