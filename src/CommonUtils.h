#ifndef _NA_DLY_COMUTL_H_
#define _NA_DLY_COMUTL_H_

#include <vector>

namespace NA {

class Circuit;
class CellArc;
class LibData;
class SimResult;
class Simulator;

std::vector<const CellArc*>
setTerminationCondition(const Circuit* ckt, const CellArc* driverArc, 
                        bool isRiseOnDriverPin, Simulator& sim);

void
measureVoltage(const SimResult& result, size_t nodeId, const LibData* libData,  
               double& delay, double& trans);

void markSimulationScope(size_t devId, Circuit* ckt);
}

#endif