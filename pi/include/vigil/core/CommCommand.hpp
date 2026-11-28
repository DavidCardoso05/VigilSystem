#pragma once
// CommCommand: what AlarmControllerThread hands
// to CommThread through the CommQueue.

#include <cstdint>
#include <variant>

namespace vigil {

//EVET_LOG.event_type
enum class EventLogType : std::uint8_t{
    ARMED, DISARMED, 
    WRONG_PIN, PIN_CHANGED, PIN_LOCKOUT, 
    INTRUSION_ALARM, ALARM_SILENCED, 
    FIRE_ALARM, HAZARD_CLEARED, 
    POWER_LOST, POWER_RESTORED, LOW_BATTERY, 
    SHUTDOWN, FAULT,
};

enum class EventLogDetail : std::uint8_t {
    NONE, LOCAL, REMOTE, IR1, IR2
};

// Insert one EVENT_LOG row as PENDING. 
//Alerts and log events take the same path:
// Protocol chooses the topic (alert or event) from the type
struct RecordEvent{
    EventLogType type;
    EventLogDetail detail = EventLogDetail::NONE;
};

// Field by field. With one operator== per alternative, std::variant compares two
// CommCommands directly (same alternative, then this operator). Used by the tests.
inline bool operator==(const RecordEvent& a, const RecordEvent& b) {
    return a.type == b.type && a.detail == b.detail;
}

// Status snapshot values; 
//CommThread adds the timestamp.
enum class ArmedStatus : std::uint8_t {
    DISARMED, ARMED, INTRUSION_ALARM //the app doesnt see all the state machines states
};

enum class FireStatus : std::uint8_t {
    NORMAL, FIRE_ALARM, SILENCED
};

enum class PowerStatus : std::uint8_t{
    GRID, BATTERY
};

// Publish the retained status 
//(on every change, never periodically).
struct PublishStatus{
    ArmedStatus armed = ArmedStatus::DISARMED;
    FireStatus fire = FireStatus::NORMAL;
    PowerStatus power = PowerStatus::GRID;
};

// Field by field. AlarmLogic publishes a new status only when this returns false
// against the last one sent. C++17 does not derive != from ==.
inline bool operator==(const PublishStatus& a, const PublishStatus& b) {
    return a.armed == b.armed && a.fire == b.fire && a.power == b.power;
}

// Later slices add: Reply, SavePin, EnterSafeState.
using CommCommand = std::variant<RecordEvent, PublishStatus>;


}  // namespace vigil
