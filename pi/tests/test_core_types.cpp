// L1 tests for the core value types (Event, CommCommand).
#include <gtest/gtest.h>

#include <variant>

#include "vigil/core/CommCommand.hpp"
#include "vigil/core/Event.hpp"

using namespace vigil;

TEST(CoreTypes, EventDefaultsToTickWithNoSource) {
    const Event ev{};
    EXPECT_EQ(ev.type, EventType::EVT_TICK);
    EXPECT_EQ(ev.source, EventSource::NONE);
    EXPECT_EQ(ev.timestamp, Time{});
}

TEST(CoreTypes, CommCommandHoldsRecordEvent) {
    const CommCommand cmd = RecordEvent{EventLogType::ARMED, EventLogDetail::REMOTE};
    ASSERT_TRUE(std::holds_alternative<RecordEvent>(cmd));
    const auto& rec = std::get<RecordEvent>(cmd);
    EXPECT_EQ(rec.type, EventLogType::ARMED);
    EXPECT_EQ(rec.detail, EventLogDetail::REMOTE);
}

TEST(CoreTypes, RecordEventDetailDefaultsToNone) {
    const RecordEvent rec{EventLogType::SHUTDOWN};
    EXPECT_EQ(rec.detail, EventLogDetail::NONE);
}

TEST(CoreTypes, CommCommandHoldsPublishStatus) {
    const CommCommand cmd = PublishStatus{ArmedStatus::INTRUSION_ALARM, FireStatus::SILENCED,
                                          PowerStatus::BATTERY};
    ASSERT_TRUE(std::holds_alternative<PublishStatus>(cmd));
    const auto& st = std::get<PublishStatus>(cmd);
    EXPECT_EQ(st.armed, ArmedStatus::INTRUSION_ALARM);
    EXPECT_EQ(st.fire, FireStatus::SILENCED);
    EXPECT_EQ(st.power, PowerStatus::BATTERY);
}

TEST(CoreTypes, PublishStatusDefaults) {
    const PublishStatus st{};
    EXPECT_EQ(st.armed, ArmedStatus::DISARMED);
    EXPECT_EQ(st.fire, FireStatus::NORMAL);
    EXPECT_EQ(st.power, PowerStatus::GRID);
}
