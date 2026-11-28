// L1 tests for AlarmLogic. Time is injected.
// Intrusion transitions are covered by test_intrusion_fsm.cpp; these tests cover only what
// AlarmLogic adds on top: the status snapshot (when it is published, and in which order),
// the outputs for each state, the deadline it reports, and the door-to-tone scenario.
#include <gtest/gtest.h>

#include <cstddef>
#include <initializer_list>

#include "vigil/logic/AlarmLogic.hpp"

using namespace vigil;

namespace {

const Time T0{};
Time at(long ms) { return T0 + Duration{ms}; }

constexpr Duration kExit{60000};
constexpr Duration kEntry{30000};

Event ev(EventType type, EventSource src = EventSource::NONE) { return Event{type, src, T0}; }
const Event kTick = ev(EventType::EVT_TICK);
const Event kButton = ev(EventType::EVT_BUTTON_PRESS);
const Event kRemoteArm = ev(EventType::EVT_REMOTE_ARM, EventSource::REMOTE);
const Event kIr1 = ev(EventType::EVT_MOTION_DETECTED, EventSource::IR1);
const Event kPinLocal = ev(EventType::EVT_PIN_CORRECT, EventSource::LOCAL);

// Builds an AlarmLogic already in the requested state (time base T0), reached through
// handle() as in the real system. The boot status is discarded, so the next handle()
// in a test shows only what that call produces.
AlarmLogic makeLogicIn(IntrusionState s) {
    const bool armedSide = s == IntrusionState::ARMED || s == IntrusionState::ENTRY_DELAY ||
                           s == IntrusionState::INTRUSION_ALARM;
    AlarmLogic logic{IntrusionConfig{},
                     armedSide ? IntrusionState::ARMED : IntrusionState::DISARMED};
    logic.handle(kTick, T0);  // boot status

    switch (s) {
        case IntrusionState::DISARMED:
        case IntrusionState::ARMED:
            break;
        case IntrusionState::EXIT_DELAY:
            logic.handle(kButton, T0);
            break;
        case IntrusionState::ENTRY_DELAY:
            logic.handle(kIr1, T0);
            break;
        case IntrusionState::INTRUSION_ALARM:
            logic.handle(kIr1, T0);
            logic.handle(kTick, T0 + kEntry);
            break;
    }
    return logic;
}

// Checks the commands and their order. ASSERT on the count: a wrong count would make the
// loop read unused slots.
void expectComm(const AlarmActions& a, std::initializer_list<CommCommand> expected) {
    ASSERT_EQ(a.commCount, expected.size());
    std::size_t i = 0;
    for (const CommCommand& e : expected) {
        EXPECT_TRUE(a.comm[i] == e) << "comm[" << i << "] differs";
        ++i;
    }
}

void expectBeep(const AlarmActions& a, Beep expected) {
    ASSERT_TRUE(a.outputs.beep.has_value());
    EXPECT_EQ(*a.outputs.beep, expected);
}

}  // namespace

// ---- boot status ------------------------------------------------------------

TEST(AlarmLogic, FirstTickPublishesDisarmedStatus) {
    AlarmLogic logic{IntrusionConfig{}, IntrusionState::DISARMED};
    const AlarmActions a = logic.handle(kTick, T0);
    expectComm(a, {PublishStatus{ArmedStatus::DISARMED, FireStatus::NORMAL, PowerStatus::GRID}});
}

TEST(AlarmLogic, FirstTickPublishesArmedWhenRestoredArmed) {
    AlarmLogic logic{IntrusionConfig{}, IntrusionState::ARMED};
    const AlarmActions a = logic.handle(kTick, T0);
    expectComm(a, {PublishStatus{ArmedStatus::ARMED}});
}

TEST(AlarmLogic, SecondTickPublishesNothing) {
    AlarmLogic logic = makeLogicIn(IntrusionState::DISARMED);
    const AlarmActions a = logic.handle(kTick, at(1000));
    expectComm(a, {});
}

// ---- status only on a change the app can see --------------------------------

TEST(AlarmLogic, ButtonStartsExitDelayWithoutStatus) {
    AlarmLogic logic = makeLogicIn(IntrusionState::DISARMED);
    const AlarmActions a = logic.handle(kButton, at(1000));
    EXPECT_EQ(logic.intrusionState(), IntrusionState::EXIT_DELAY);
    expectComm(a, {});  // the app still sees DISARMED
    expectBeep(a, Beep::KEY);
    EXPECT_EQ(a.outputs.speaker, SpeakerBase::OFF);  // the exit delay has no tone
    EXPECT_EQ(a.outputs.redLed, RedLedMode::ON);
}

TEST(AlarmLogic, ExitDeadlineArmsLocallyThenPublishes) {
    AlarmLogic logic = makeLogicIn(IntrusionState::EXIT_DELAY);
    const AlarmActions a = logic.handle(kTick, T0 + kExit);
    EXPECT_EQ(logic.intrusionState(), IntrusionState::ARMED);
    expectComm(a, {RecordEvent{EventLogType::ARMED, EventLogDetail::LOCAL},
                   PublishStatus{ArmedStatus::ARMED}});
}

TEST(AlarmLogic, MotionInArmedStartsEntryDelayWithoutComm) {
    AlarmLogic logic = makeLogicIn(IntrusionState::ARMED);
    const AlarmActions a = logic.handle(kIr1, at(1000));
    EXPECT_EQ(logic.intrusionState(), IntrusionState::ENTRY_DELAY);
    expectComm(a, {});  // no alert, no log, and the app still sees ARMED
    EXPECT_EQ(a.outputs.speaker, SpeakerBase::ENTRY_WARNING);
    EXPECT_EQ(logic.nextDeadline(), at(1000) + kEntry);
}

TEST(AlarmLogic, PinInExitDelayDisarmsWithoutStatus) {
    AlarmLogic logic = makeLogicIn(IntrusionState::EXIT_DELAY);
    const AlarmActions a = logic.handle(kPinLocal, at(1000));
    EXPECT_EQ(logic.intrusionState(), IntrusionState::DISARMED);
    // Logged, but the status was already DISARMED for the app.
    expectComm(a, {RecordEvent{EventLogType::DISARMED, EventLogDetail::LOCAL}});
}

TEST(AlarmLogic, RemoteArmRecordsAndPublishesWithoutReply) {
    AlarmLogic logic = makeLogicIn(IntrusionState::DISARMED);
    const AlarmActions a = logic.handle(kRemoteArm, at(1000));
    EXPECT_EQ(logic.intrusionState(), IntrusionState::ARMED);
    expectComm(a, {RecordEvent{EventLogType::ARMED, EventLogDetail::REMOTE},
                   PublishStatus{ArmedStatus::ARMED}});
}

// ---- outputs ----------------------------------------------------------------

TEST(AlarmLogic, ButtonInEntryDelayPlaysErrorBeepOverWarning) {
    AlarmLogic logic = makeLogicIn(IntrusionState::ENTRY_DELAY);
    const AlarmActions a = logic.handle(kButton, at(1000));
    expectBeep(a, Beep::ERROR);
    EXPECT_EQ(a.outputs.speaker, SpeakerBase::ENTRY_WARNING);
}

TEST(AlarmLogic, ButtonInIntrusionAlarmDropsBeep) {
    AlarmLogic logic = makeLogicIn(IntrusionState::INTRUSION_ALARM);
    const AlarmActions a = logic.handle(kButton, at(40000));
    EXPECT_FALSE(a.outputs.beep.has_value());
    EXPECT_EQ(a.outputs.speaker, SpeakerBase::INTRUSION);
}

TEST(AlarmLogic, IntrusionAlarmOutputs) {
    AlarmLogic logic = makeLogicIn(IntrusionState::ENTRY_DELAY);
    const AlarmActions a = logic.handle(kTick, T0 + kEntry);
    EXPECT_EQ(a.outputs.speaker, SpeakerBase::INTRUSION);
    EXPECT_EQ(a.outputs.redLed, RedLedMode::FLASHING);
}

TEST(AlarmLogic, RgbIsYellowWithMainsAndNoBroker) {
    AlarmLogic logic = makeLogicIn(IntrusionState::DISARMED);
    const AlarmActions a = logic.handle(kTick, at(1000));
    EXPECT_EQ(a.outputs.rgb, RgbColor::YELLOW);  // the network starts lost
}

// ---- deadlines --------------------------------------------------------------

TEST(AlarmLogic, NextDeadlineFollowsIntrusionFsm) {
    EXPECT_FALSE(makeLogicIn(IntrusionState::DISARMED).nextDeadline().has_value());
    EXPECT_FALSE(makeLogicIn(IntrusionState::ARMED).nextDeadline().has_value());
    EXPECT_EQ(makeLogicIn(IntrusionState::EXIT_DELAY).nextDeadline(), T0 + kExit);
    EXPECT_EQ(makeLogicIn(IntrusionState::ENTRY_DELAY).nextDeadline(), T0 + kEntry);
    EXPECT_FALSE(makeLogicIn(IntrusionState::INTRUSION_ALARM).nextDeadline().has_value());
}

// ---- end to end (slice 1 integration point) ---------------------------------

TEST(AlarmLogic, DoorOpensWhileArmedEndsInIntrusionTone) {
    AlarmLogic logic{IntrusionConfig{}, IntrusionState::ARMED};
    logic.handle(kTick, T0);  // boot status

    const AlarmActions entry = logic.handle(kIr1, at(1000));
    expectComm(entry, {});
    EXPECT_EQ(entry.outputs.speaker, SpeakerBase::ENTRY_WARNING);

    const AlarmActions alarm = logic.handle(kTick, at(1000) + kEntry);
    EXPECT_EQ(alarm.outputs.speaker, SpeakerBase::INTRUSION);
    expectComm(alarm, {RecordEvent{EventLogType::INTRUSION_ALARM, EventLogDetail::IR1},
                       PublishStatus{ArmedStatus::INTRUSION_ALARM}});
}

TEST(AlarmLogic, PinDuringIntrusionStopsEverything) {
    AlarmLogic logic = makeLogicIn(IntrusionState::INTRUSION_ALARM);
    const AlarmActions a = logic.handle(kPinLocal, at(40000));
    EXPECT_EQ(logic.intrusionState(), IntrusionState::DISARMED);
    expectComm(a, {RecordEvent{EventLogType::DISARMED, EventLogDetail::LOCAL},
                   PublishStatus{ArmedStatus::DISARMED}});
    EXPECT_EQ(a.outputs.speaker, SpeakerBase::OFF);
    EXPECT_EQ(a.outputs.redLed, RedLedMode::OFF);
    EXPECT_FALSE(logic.nextDeadline().has_value());
}
