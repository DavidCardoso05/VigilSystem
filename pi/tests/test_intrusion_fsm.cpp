// L1 tests for IntrusionFsm (Master Context §E.4). Time is injected.
#include <gtest/gtest.h>

#include <iterator>
#include <stdexcept>
#include <utility>
#include <variant>

#include "vigil/logic/IntrusionFsm.hpp"

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
const Event kIr2 = ev(EventType::EVT_MOTION_DETECTED, EventSource::IR2);
const Event kPinLocal = ev(EventType::EVT_PIN_CORRECT, EventSource::LOCAL);
const Event kPinRemote = ev(EventType::EVT_PIN_CORRECT, EventSource::REMOTE);

void expectNoComm(const FsmActions& a) { EXPECT_EQ(a.commCount, 0u); }

void expectSingleRecord(const FsmActions& a, EventLogType type, EventLogDetail detail) {
    ASSERT_EQ(a.commCount, 1u);
    ASSERT_TRUE(std::holds_alternative<RecordEvent>(a.comm[0]));
    const auto& rec = std::get<RecordEvent>(a.comm[0]);
    EXPECT_EQ(rec.type, type);
    EXPECT_EQ(rec.detail, detail);
}

// Builds an FSM already in the requested state (time base T0).
IntrusionFsm makeIn(IntrusionState s) {
    switch (s) {
        case IntrusionState::DISARMED:
        case IntrusionState::ARMED:
            return IntrusionFsm{IntrusionConfig{}, s};
        case IntrusionState::EXIT_DELAY: {
            IntrusionFsm f{IntrusionConfig{}, IntrusionState::DISARMED};
            f.onEvent(kButton, T0, false);
            return f;
        }
        case IntrusionState::ENTRY_DELAY: {
            IntrusionFsm f{IntrusionConfig{}, IntrusionState::ARMED};
            f.onEvent(kIr1, T0, false);
            return f;
        }
        case IntrusionState::INTRUSION_ALARM: {
            IntrusionFsm f{IntrusionConfig{}, IntrusionState::ARMED};
            f.onEvent(kIr1, T0, false);
            f.onEvent(kTick, T0 + kEntry, false);
            return f;
        }
    }
    throw std::logic_error("unreachable");
}

}  // namespace

// ---- construction -----------------------------------------------------------

TEST(IntrusionFsm, ValidInitialStates) {
    IntrusionFsm d{IntrusionConfig{}, IntrusionState::DISARMED};
    EXPECT_EQ(d.state(), IntrusionState::DISARMED);
    EXPECT_FALSE(d.nextDeadline().has_value());

    IntrusionFsm a{IntrusionConfig{}, IntrusionState::ARMED};
    EXPECT_EQ(a.state(), IntrusionState::ARMED);
    EXPECT_FALSE(a.nextDeadline().has_value());
}

TEST(IntrusionFsm, InvalidInitialStatesThrow) {
    for (auto s : {IntrusionState::EXIT_DELAY, IntrusionState::ENTRY_DELAY,
                   IntrusionState::INTRUSION_ALARM}) {
        EXPECT_THROW((IntrusionFsm{IntrusionConfig{}, s}), std::invalid_argument);
    }
}

// ---- arm button -------------------------------------------------------------

TEST(IntrusionFsm, ButtonInDisarmedStartsExitDelay) {
    IntrusionFsm f{IntrusionConfig{}, IntrusionState::DISARMED};
    const auto a = f.onEvent(kButton, at(1000), false);
    EXPECT_EQ(f.state(), IntrusionState::EXIT_DELAY);
    EXPECT_EQ(a.feedback, Feedback::KEY_BEEP);
    expectNoComm(a);
    ASSERT_TRUE(f.nextDeadline().has_value());
    EXPECT_EQ(*f.nextDeadline(), at(1000) + kExit);
}

TEST(IntrusionFsm, ButtonInExitDelayIsErrorAndKeepsDeadline) {
    auto f = makeIn(IntrusionState::EXIT_DELAY);
    const auto a = f.onEvent(kButton, at(5000), false);
    EXPECT_EQ(f.state(), IntrusionState::EXIT_DELAY);
    EXPECT_EQ(a.feedback, Feedback::ERROR_BEEP);
    expectNoComm(a);
    ASSERT_TRUE(f.nextDeadline().has_value());
    EXPECT_EQ(*f.nextDeadline(), T0 + kExit);
}

TEST(IntrusionFsm, ButtonInOtherStatesIsError) {
    for (auto s : {IntrusionState::ARMED, IntrusionState::ENTRY_DELAY,
                   IntrusionState::INTRUSION_ALARM}) {
        auto f = makeIn(s);
        const auto before = f.nextDeadline();
        const auto a = f.onEvent(kButton, at(1), false);
        EXPECT_EQ(f.state(), s);
        EXPECT_EQ(a.feedback, Feedback::ERROR_BEEP);
        EXPECT_EQ(f.nextDeadline(), before);
        expectNoComm(a);
    }
}

TEST(IntrusionFsm, ButtonDuringFireIsErrorAndStaysDisarmed) {
    IntrusionFsm f{IntrusionConfig{}, IntrusionState::DISARMED};
    const auto a = f.onEvent(kButton, T0, true);
    EXPECT_EQ(f.state(), IntrusionState::DISARMED);
    EXPECT_EQ(a.feedback, Feedback::ERROR_BEEP);
    EXPECT_FALSE(f.nextDeadline().has_value());
    expectNoComm(a);
}

// ---- exit deadline ----------------------------------------------------------

TEST(IntrusionFsm, ExitDeadlineArmsLocallyExactlyAt60s) {
    auto f = makeIn(IntrusionState::EXIT_DELAY);

    auto a = f.onEvent(kTick, at(59999), false);
    EXPECT_EQ(f.state(), IntrusionState::EXIT_DELAY);
    expectNoComm(a);

    a = f.onEvent(kTick, at(60000), false);
    EXPECT_EQ(f.state(), IntrusionState::ARMED);
    EXPECT_FALSE(f.nextDeadline().has_value());
    EXPECT_EQ(a.feedback, Feedback::NONE);
    expectSingleRecord(a, EventLogType::ARMED, EventLogDetail::LOCAL);
}

TEST(IntrusionFsm, ExitDeadlineDuringFireStaysAndClearsDeadline) {
    auto f = makeIn(IntrusionState::EXIT_DELAY);
    const auto a = f.onEvent(kTick, at(60000), true);
    EXPECT_EQ(f.state(), IntrusionState::EXIT_DELAY);
    EXPECT_FALSE(f.nextDeadline().has_value());
    expectNoComm(a);

    // Later ticks do nothing (no busy loop, no late arming).
    const auto b = f.onEvent(kTick, at(120000), false);
    EXPECT_EQ(f.state(), IntrusionState::EXIT_DELAY);
    expectNoComm(b);
}

// ---- remote arm -------------------------------------------------------------

TEST(IntrusionFsm, RemoteArmFromDisarmed) {
    IntrusionFsm f{IntrusionConfig{}, IntrusionState::DISARMED};
    const auto a = f.onEvent(kRemoteArm, T0, false);
    EXPECT_EQ(f.state(), IntrusionState::ARMED);
    EXPECT_FALSE(f.nextDeadline().has_value());
    expectSingleRecord(a, EventLogType::ARMED, EventLogDetail::REMOTE);
}

TEST(IntrusionFsm, RemoteArmFromExitDelayCancelsDeadline) {
    auto f = makeIn(IntrusionState::EXIT_DELAY);
    const auto a = f.onEvent(kRemoteArm, at(10000), false);
    EXPECT_EQ(f.state(), IntrusionState::ARMED);
    EXPECT_FALSE(f.nextDeadline().has_value());
    expectSingleRecord(a, EventLogType::ARMED, EventLogDetail::REMOTE);
}

TEST(IntrusionFsm, RemoteArmWhenArmedDoesNothing) {
    auto f = makeIn(IntrusionState::ARMED);
    const auto a = f.onEvent(kRemoteArm, T0, false);
    EXPECT_EQ(f.state(), IntrusionState::ARMED);
    EXPECT_EQ(a.feedback, Feedback::NONE);
    expectNoComm(a);
}

TEST(IntrusionFsm, RemoteArmDuringFireDoesNothing) {
    for (auto s : {IntrusionState::DISARMED, IntrusionState::EXIT_DELAY}) {
        auto f = makeIn(s);
        const auto before = f.nextDeadline();
        const auto a = f.onEvent(kRemoteArm, at(1), true);
        EXPECT_EQ(f.state(), s);
        EXPECT_EQ(f.nextDeadline(), before);
        expectNoComm(a);
    }
}

// ---- motion -----------------------------------------------------------------

TEST(IntrusionFsm, MotionInArmedStartsEntryDelay) {
    IntrusionFsm f{IntrusionConfig{}, IntrusionState::ARMED};
    const auto a = f.onEvent(kIr2, at(2000), false);
    EXPECT_EQ(f.state(), IntrusionState::ENTRY_DELAY);
    ASSERT_TRUE(f.nextDeadline().has_value());
    EXPECT_EQ(*f.nextDeadline(), at(2000) + kEntry);
    EXPECT_EQ(a.feedback, Feedback::NONE);
    expectNoComm(a);
}

TEST(IntrusionFsm, MotionIgnoredInOtherStates) {
    for (auto s : {IntrusionState::DISARMED, IntrusionState::EXIT_DELAY,
                   IntrusionState::ENTRY_DELAY, IntrusionState::INTRUSION_ALARM}) {
        auto f = makeIn(s);
        const auto before = f.nextDeadline();
        const auto a = f.onEvent(kIr2, at(1), false);
        EXPECT_EQ(f.state(), s);
        EXPECT_EQ(f.nextDeadline(), before);
        EXPECT_EQ(a.feedback, Feedback::NONE);
        expectNoComm(a);
    }
}

TEST(IntrusionFsm, EntryDeadlineRaisesAlarmWithFirstSensor) {
    IntrusionFsm f{IntrusionConfig{}, IntrusionState::ARMED};
    f.onEvent(kIr2, T0, false);
    f.onEvent(kIr1, at(1000), false);  // ignored in ENTRY_DELAY

    auto a = f.onEvent(kTick, at(29999), false);
    EXPECT_EQ(f.state(), IntrusionState::ENTRY_DELAY);
    expectNoComm(a);

    a = f.onEvent(kTick, at(30000), false);
    EXPECT_EQ(f.state(), IntrusionState::INTRUSION_ALARM);
    EXPECT_FALSE(f.nextDeadline().has_value());
    expectSingleRecord(a, EventLogType::INTRUSION_ALARM, EventLogDetail::IR2);
}

// ---- correct PIN ------------------------------------------------------------

TEST(IntrusionFsm, PinCorrectDisarmsFromEveryActiveState) {
    for (auto s : {IntrusionState::EXIT_DELAY, IntrusionState::ARMED,
                   IntrusionState::ENTRY_DELAY, IntrusionState::INTRUSION_ALARM}) {
        for (const auto& [pin, detail] :
             {std::pair{kPinLocal, EventLogDetail::LOCAL},
              std::pair{kPinRemote, EventLogDetail::REMOTE}}) {
            auto f = makeIn(s);
            const auto a = f.onEvent(pin, at(1), false);
            EXPECT_EQ(f.state(), IntrusionState::DISARMED);
            EXPECT_FALSE(f.nextDeadline().has_value());
            expectSingleRecord(a, EventLogType::DISARMED, detail);
        }
    }
}

TEST(IntrusionFsm, PinCorrectInDisarmedDoesNothing) {
    for (const auto& pin : {kPinLocal, kPinRemote}) {
        IntrusionFsm f{IntrusionConfig{}, IntrusionState::DISARMED};
        const auto a = f.onEvent(pin, T0, false);
        EXPECT_EQ(f.state(), IntrusionState::DISARMED);
        EXPECT_EQ(a.feedback, Feedback::NONE);
        expectNoComm(a);
    }
}

// ---- deadline then event in one call ----------------------------------------

TEST(IntrusionFsm, MotionAfterExpiredExitDeadlineArmsThenEntersEntryDelay) {
    auto f = makeIn(IntrusionState::EXIT_DELAY);
    const auto a = f.onEvent(kIr1, at(61000), false);
    EXPECT_EQ(f.state(), IntrusionState::ENTRY_DELAY);
    ASSERT_TRUE(f.nextDeadline().has_value());
    EXPECT_EQ(*f.nextDeadline(), at(61000) + kEntry);
    expectSingleRecord(a, EventLogType::ARMED, EventLogDetail::LOCAL);
}

TEST(IntrusionFsm, NeverEmitsPublishStatus) {
    IntrusionFsm f{IntrusionConfig{}, IntrusionState::DISARMED};
    const Event seq[] = {kButton, kTick, kIr1, kTick, kPinLocal, kRemoteArm, kPinRemote};
    const long times[] = {0, 60000, 61000, 91000, 92000, 93000, 94000};
    for (std::size_t i = 0; i < std::size(seq); ++i) {
        const auto a = f.onEvent(seq[i], at(times[i]), false);
        for (std::size_t c = 0; c < a.commCount; ++c) {
            EXPECT_FALSE(std::holds_alternative<PublishStatus>(a.comm[c]));
        }
    }
}

// ---- FsmActions -------------------------------------------------------------

TEST(FsmActions, AddThrowsWhenFull) {
    FsmActions a;
    for (std::size_t i = 0; i < a.comm.size(); ++i) {
        a.add(RecordEvent{EventLogType::ARMED});
    }
    EXPECT_EQ(a.commCount, a.comm.size());
    EXPECT_THROW(a.add(RecordEvent{EventLogType::ARMED}), std::logic_error);
    EXPECT_EQ(a.commCount, a.comm.size());
}
