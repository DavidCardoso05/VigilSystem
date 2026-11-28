#pragma once
// State enums and small value types shared by the logic package.

#include <cstdint>

#include "vigil/core/CommCommand.hpp"
#include "vigil/core/Time.hpp"

namespace vigil {

enum class IntrusionState : std::uint8_t {
    DISARMED, EXIT_DELAY, ARMED, ENTRY_DELAY, INTRUSION_ALARM
};

// Field of AlarmLogic; only changes the RGB colour (Master Context §E.4).
enum class NetworkState : std::uint8_t { NET_LOST, NET_CONNECTED };

// One-shot local feedback requested by a transition (played by the outputs).
enum class Feedback : std::uint8_t { NONE, KEY_BEEP, ERROR_BEEP, WRONG_PIN_BLINK };

enum class FireState : std::uint8_t { NORMAL, FIRE_ALARM, SILENCED };

enum class PowerSource : std::uint8_t { GRID, BATTERY };

// OutputInputs (what OutputPolicy needs) is defined in vigil/logic/OutputPolicy.hpp.

struct IntrusionConfig {
    Duration exitDelay{60000};
    Duration entryDelay{30000};
};

}  // namespace vigil
