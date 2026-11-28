#include "vigil/logic/AlarmLogic.hpp"

namespace vigil {

namespace {
// The app sees three armed states. The exit delay shows as DISARMED: motion is still
// ignored, so nothing is protected yet. The entry delay shows as ARMED: it is not
// reported, so a resident who is slow with the PIN causes no alert on the phones.
// No default: a new IntrusionState makes the compiler warn here.
ArmedStatus toArmedStatus(IntrusionState s) {
    switch (s) {
        case IntrusionState::DISARMED:
        case IntrusionState::EXIT_DELAY:
            return ArmedStatus::DISARMED;
        case IntrusionState::ARMED:
        case IntrusionState::ENTRY_DELAY:
            return ArmedStatus::ARMED;
        case IntrusionState::INTRUSION_ALARM:
            return ArmedStatus::INTRUSION_ALARM;
    }
    return ArmedStatus::DISARMED;  // unreachable; keeps -Wreturn-type quiet
}
}  // namespace

AlarmLogic::AlarmLogic(IntrusionConfig cfg, IntrusionState initial)
    : intrusion_(cfg, initial) {}

AlarmActions AlarmLogic::handle(const Event& ev, Time now) {
    const FsmActions fsm = intrusion_.onEvent(ev, now, /*fireAlarmActive=*/false);

    AlarmActions out;
    for (std::size_t i = 0; i < fsm.commCount; ++i) {
        out.add(fsm.comm[i]);
    }

    // Compared with the last status sent, not derived from the transition: internal
    // changes the app cannot see (exit and entry delay) publish nothing, and new state
    // machines are covered without extra code. Empty lastStatus_ = first call.
    const PublishStatus status = snapshot();
    if (!lastStatus_ || !(status == *lastStatus_)) {
        out.add(status);
        lastStatus_ = status;
    }

    // Recomputed after every event, even when nothing changed: AlarmOutputs skips a
    // sequence that is already playing.
    out.outputs = policy_.compute(outputInputs(fsm.feedback));
    return out;
}

std::optional<Time> AlarmLogic::nextDeadline() const {
    return intrusion_.nextDeadline();
}

PublishStatus AlarmLogic::snapshot() const {
    PublishStatus s;
    s.armed = toArmedStatus(intrusion_.state());
    s.fire = FireStatus::NORMAL;   // fixed until the fire rules exist
    s.power = PowerStatus::GRID;   // fixed until the power rules exist
    return s;
}

OutputInputs AlarmLogic::outputInputs(Feedback fb) const {
    OutputInputs in;
    in.intrusion = intrusion_.state();
    in.fire = FireState::NORMAL;
    in.power = PowerSource::GRID;
    in.network = network_;

    // The FSM asks for one-shot feedback; OutputPolicy decides whether it is played
    // (beeps and blinks are dropped while an alarm sounds or the LED flashes).
    switch (fb) {
        case Feedback::KEY_BEEP:        in.beep = Beep::KEY; break;
        case Feedback::ERROR_BEEP:      in.beep = Beep::ERROR; break;
        case Feedback::WRONG_PIN_BLINK: in.wrongPinBlink = true; break;
        case Feedback::NONE:            break;
    }
    return in;
}

}  // namespace vigil
