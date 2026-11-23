#pragma once

#include <optional>
#include "vigil/logic/States.hpp"
#include "vigil/logic/DesiredOutputs.hpp"

namespace vigil {
    struct OutputInputs {
        IntrusionState intrusion= IntrusionState::DISARMED;
        FireState fire= FireState::NORMAL;
        PowerSource power= PowerSource::GRID;
        NetworkState network= NetworkState::NET_LOST;
        std::optional <Beep> beep;
        bool wrongPinBlink= false;

    };

class OutputPolicy {
    public:
        DesiredOutputs compute(const OutputInputs& in) const;
    };

}  // namespace vigil
