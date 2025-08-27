#include "CSMDelay.h"
#include "CSMCellDelay.h"
#include "Simulator.h"
#include "SimResult.h"
#include "Debug.h"
#include "CommonUtils.h"
#include "Plotter.h"

namespace NA {

CSMDelay::CSMDelay(const AnalysisParameter& param, const NetlistParser& parser, bool isMaxDelay)
: _isMaxDelay(isMaxDelay), _ckt(parser, param)
{
  const std::vector<std::string>& pinsToCalc = parser.cellOutPinsToCalcDelay();
  for (const std::string& outPin : pinsToCalc) {
    const std::vector<std::string>& cellInPins = _ckt.cellArcFromPins(outPin);
    for (const std::string& frPin : cellInPins) {
      const CellArc* driverArc = _ckt.cellArc(frPin, outPin);
      if (driverArc == nullptr) {
        printf("ERROR: Cannot find cell arc connected on pin %s\n", frPin.data());
        continue;
      }
      _cellArcs.push_back(driverArc);
    }
  }
}

void
CSMDelay::calculate()
{
  for (const CellArc* driverArc : _cellArcs) {
    calculateArc(driverArc);
  }
}

void
CSMDelay::calculateArc(const CellArc* driverArc)
{
  CSMCellDelay cellDelayCalc(driverArc, &_ckt, _isMaxDelay);
  cellDelayCalc.calculate();
  const SimResult& simResult = cellDelayCalc.result();
  const LibData* libData = driverArc->libData();
  //const Device& inputSrc = _ckt.device(driverArc->inputSourceDevId(&_ckt));
  size_t inputNodeId = driverArc->inputNode();
  double inputT50;
  double inputTran;
  measureVoltage(simResult, inputNodeId, libData, inputT50, inputTran);
  size_t outputNodeId = driverArc->outputNode(&_ckt);
  double outputT50;
  double outputTran;
  measureVoltage(simResult, outputNodeId, libData, outputT50, outputTran);
  double cellDelay = outputT50 - inputT50;
  printf("Cell delay of %s:%s->%s: %G, transition on output pin: %G\n", driverArc->instance().data(), driverArc->fromPin().data(), 
          driverArc->toPin().data(), cellDelay, outputTran);
  if (Debug::enabled(DebugModule::CCS)) {
    PlotData cellArcPlotData;
    cellArcPlotData._canvasName = "Cell Delay";
    populatePlotData(cellArcPlotData, driverArc->inputNode(), driverArc->outputNode(&_ckt), &_ckt);
    Plotter::plot(cellArcPlotData, {_ckt}, {simResult});
  }
  const std::vector<const CellArc*>& loadArcs = cellDelayCalc.loadArcs();
  for (const CellArc* loadArc : loadArcs) {
    size_t loadNode = loadArc->inputNode();
    double loadT50;
    double loadTran;
    measureVoltage(simResult, loadNode, loadArc->libData(), loadT50, loadTran);
    double netDelay = loadT50 - outputT50;
    printf("Net delay of %s->%s: %G, transition on %s: %G\n", driverArc->toPinFullName().data(), 
           loadArc->fromPinFullName().data(), netDelay, loadArc->fromPinFullName().data(), loadTran);
    if (Debug::enabled(DebugModule::CCS)) {
      PlotData netArcPlotData;
      netArcPlotData._canvasName = "Net Delay";
      populatePlotData(netArcPlotData, driverArc->outputNode(&_ckt), loadArc->inputNode(), &_ckt);
      Plotter::plot(netArcPlotData, {_ckt}, {simResult});
    }
  }
}

}