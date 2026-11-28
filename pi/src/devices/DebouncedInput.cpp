#include "vigil/devices/DebouncedInput.hpp"

namespace vigil {

DebouncedInput::DebouncedInput(IGpioInput& line, DebouncedInputConfig config)
    : line_(line), config_(config) {
    if (config_.confirmSamples == 0) {
        config_.confirmSamples = 1;
    }
}

InputEdge DebouncedInput::sample(Time now) {
    if (!started_) {
        started_ = true;
        start_ = now;
    }
    if (now - start_ < config_.warmup) {
        return InputEdge::NONE;
    }

    const bool reading = (line_.read() == config_.activeLevel);

    if (!baselined_) {
        active_ = reading;
        baselined_ = true;
        pending_ = 0;
        return InputEdge::NONE;
    }

    if (reading == active_) {
        pending_ = 0;
        return InputEdge::NONE;
    }

    ++pending_;
    if (pending_ < config_.confirmSamples) {
        return InputEdge::NONE;
    }

    active_ = reading;
    pending_ = 0;
    return active_ ? InputEdge::ACTIVATED : InputEdge::DEACTIVATED;
}

}  // namespace vigil
