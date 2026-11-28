#pragma once
// Event: small value type carried by
// the EventQueue to AlarmControllerThread.

// TO ADD THE OTHER EVENTS WHEN THE TIME COMES!!

#include <cstdint>

#include "vigil/core/Time.hpp"

namespace vigil {
enum class EventType : std::uint8_t { 
    EVT_TICK, // wait time runs out without an event
    EVT_BUTTON_PRESS, 
    EVT_MOTION_DETECTED,
    EVT_REMOTE_ARM,
    EVT_PIN_CORRECT
};

enum class EventSource : std::uint8_t {
    NONE, IR1, IR2, LOCAL, REMOTE
};

struct Event {
    EventType type = EventType::EVT_TICK; //type omission value is the evt_tick
    EventSource source = EventSource::NONE;
    Time timestamp{};
};
}  // namespace vigil
