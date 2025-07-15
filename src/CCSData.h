#include <vector>
#include "LibData.h"
#include "SimResult.h"  /// For Waveform

namespace NA {

class CCSDriverData {
	public:
		CCSDriverData(const CCSArc* cellArc, bool isRise);


	private:
		initVoltageWaveforms(const CCSGroup& luts);


	private:
		bool   								_isRise;
		const CCSArc*         _arc;
		std::vector<Waveform> _voltageWaveforms;

};




}