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

    // Later slices add: Reply, SavePin, EnterSafeState.
    using CommCommand = std::variant<RecordEvent, PublishStatus>;


} //namespace vigil