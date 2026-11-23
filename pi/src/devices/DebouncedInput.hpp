#pragma once
// DebouncedInput: turns a noisy digital line into confirmed activations (Master
// Context §E.3). One class, four instances: IR1, IR2, MQ-2 and the arm button.
//
// Rules:
//  - the line is sampled by the caller (periodic thread); this class never sleeps;
//  - a change is confirmed only after `confirmSamples` consecutive reads of the new
//    level (IR 2, button 2, MQ-2 5); a shorter glitch is ignored;
//  - the first reading after the warm-up is the baseline: it emits nothing, so a door
//    already open at boot does not trigger (§E.7);
//  - during the warm-up the line is not read and nothing is emitted (MQ-2, 60 s);
//  - one event per change: a sustained level never repeats an event.

#include <cstdint>

#include "vigil/core/Time.hpp"
#include "vigil/hw/IGpioInput.hpp"

namespace vigil {

    struct DebouncedInputConfig {
        unsigned confirmSamples = 2;           // consecutive reads needed (0 is treated as 1)
        Level activeLevel = Level::LOW;        // level that means "active"
        Duration warmup{0};                    // from the first sample() call
    };

    enum class InputEdge : std::uint8_t { NONE, ACTIVATED, DEACTIVATED };

    class DebouncedInput {
    public:
        DebouncedInput(IGpioInput& line, DebouncedInputConfig config);

        // Reads the line once (unless still warming up) and reports a confirmed change.
        InputEdge sample(Time now);

        // Confirmed state. Meaningful only when ready().
        bool active() const { return active_; }

        // True once the baseline has been taken (warm-up over, first reading done).
        bool ready() const { return baselined_; }

    private:
        IGpioInput& line_;
        DebouncedInputConfig config_;

        bool started_ = false;
        Time start_{};
        bool baselined_ = false;
        bool active_ = false;
        unsigned pending_ = 0;   // consecutive reads that differ from the confirmed state
    };

} // namespace vigil