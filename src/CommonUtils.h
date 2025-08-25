#ifndef _NA_DLY_COMUTL_H_
#define _NA_DLY_COMUTL_H_

namespace NA {

std::vector<const CellArc*>
setTerminationCondition(const Circuit* ckt, const CellArc* driverArc, 
                        bool isRiseOnDriverPin, Simulator& sim);

void
measureVoltage(const SimResult& result, size_t nodeId, const LibData* libData,  
               double& delay, double& trans);

}

#endif