#pragma once
// Pure logic:
// no I/O, no clock reads (time is injected), no allocation in onEvent().
// Guards that need other state (fire, lockout, PIN check) are evaluated by
// AlarmLogic; this class only receives their result.

#include <optional>

#include "vigil/core/Event.hpp"
#include "vigil/core/Time.hpp"
#include "vigil/logic/FsmActions.hpp"
#include "vigil/logic/States.hpp"

namespace vigil {

class IntrusionFsm {
public:
    // initial must be ARMED or DISARMED (restored at boot, nothing logged);
    // otherwise throws std::invalid_argument.
    IntrusionFsm(IntrusionConfig cfg, IntrusionState initial);

    // (1) applies an expired deadline (now >= deadline), then (2) processes ev.
    FsmActions onEvent(const Event& ev, Time now, bool fireAlarmActive);

    IntrusionState state() const { return state_; }
    std::optional<Time> nextDeadline() const { return deadline_; }

private:
    void applyDeadline(FsmActions& out, Time now, bool fireAlarmActive);
    void handle(const Event& ev, FsmActions& out, Time now, bool fireAlarmActive);

    IntrusionConfig cfg_;
    IntrusionState state_;
    std::optional<Time> deadline_;
    EventLogDetail motionSensor_ = EventLogDetail::NONE;  // remembered on ARMED → ENTRY_DELAY
};

}  // namespace vigil
