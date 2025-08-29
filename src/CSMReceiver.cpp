#include "LibData.h"
#include "CSMReceiver.h"
#include "RampVCellDelay.h"
#include "Debug.h"

namespace NA {

CSMReceiver::CSMReceiver(Circuit* ckt, const CellArc* loadArc, bool isLoadPinRise)
: _isLoadPinRise(isLoadPinRise), _loadArc(loadArc), _ckt(ckt) 
{
  _rcvCapLUTType = LUTType::RiseRecvCap;
  if (isLoadPinRise == false) {
    _rcvCapLUTType = LUTType::FallRecvCap;
  }
}

void
CSMReceiver::calcFixedReceiverCap()
{
  double loadCap = _loadArc->fixedLoadCap(_isLoadPinRise);
  const LibData* libData = _loadArc->libData();
  const std::vector<NLDMLUT>& recvCapLUT = _loadArc->ccsData()->getRecvCap(_rcvCapLUTType);
  double voltage = libData->voltage();
  if (_isLoadPinRise == false) voltage = -voltage;
  double dv = voltage / recvCapLUT.size();
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: Receiver cap on %s is: [", _loadArc->fromPinFullName().data());
  }
  for (size_t i=0; i<recvCapLUT.size(); ++i) {
    double vThres = i*dv;
    _capThresholdVoltage.push_back(vThres);
    _recvCaps.push_back(loadCap);
    if (Debug::enabled(DebugModule::CCS)) {
      printf("{%.3f %G} ", vThres, loadCap);
    }
  }
  if (Debug::enabled(DebugModule::CCS)) {
    printf("]\n");
  }
  _capThresholdVoltage.push_back(voltage);
}

void
CSMReceiver::calcReceiverCap(const SimResult& simResult) 
{
  if (simResult.empty()) {
    calcFixedReceiverCap();
    return;
  }
  const Waveform& loadPinWaveform = simResult.nodeVoltageWaveform(_loadArc->inputTranNode());
  assert(loadPinWaveform.isRise() == _isLoadPinRise);
  const LibData* libData = _loadArc->libData();
  double inputTran = loadPinWaveform.transitionTime(libData);
  RampVCellDelay nldmCalc(_loadArc, _ckt);
  nldmCalc.setInputTransition(inputTran);
  nldmCalc.setIsInputTranRise(_isLoadPinRise);
  bool success = nldmCalc.calculate();
  assert(success == true);
  double effCap = nldmCalc.effCap();
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: EffCap connected on %s is %G\n", _loadArc->toPinFullName().data(), effCap);
  }
  const std::vector<NLDMLUT>& recvCapLUT = _loadArc->ccsData()->getRecvCap(_rcvCapLUTType);
  double voltage = libData->voltage();
  if (_isLoadPinRise == false) voltage = -voltage;
  double dv = voltage / recvCapLUT.size();
  if (Debug::enabled(DebugModule::CCS)) {
    printf("DEBUG: Receiver cap on %s is: [", _loadArc->fromPinFullName().data());
  }
  for (size_t i=0; i<recvCapLUT.size(); ++i) {
    double vThres = i*dv;
    _capThresholdVoltage.push_back(vThres);
    const NLDMLUT& lut = recvCapLUT[i];
    double cValue = lut.value(inputTran, effCap);
    _recvCaps.push_back(cValue);
    if (Debug::enabled(DebugModule::CCS)) {
      printf("{%.3f %G} ", vThres, cValue);
    }
  }
  if (Debug::enabled(DebugModule::CCS)) {
    printf("]\n");
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
      if (_isLoadPinRise) {
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
    loadCap = _loadArc->fixedLoadCap(_isLoadPinRise);
  }
  return loadCap;
}

}