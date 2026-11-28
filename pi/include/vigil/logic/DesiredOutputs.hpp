#pragma once 

#include <cstdint>
#include <optional>

namespace vigil {

enum class SpeakerBase : std::uint8_t {OFF, ENTRY_WARNING, INTRUSION, FIRE };
enum class Beep : std::uint8_t { KEY, ERROR};
enum class RedLedMode : std::uint8_t { OFF, ON, FLASHING};
enum class RgbColor : std::uint8_t { GREEN, YELLOW, BLUE, MAGENTA };

struct DesiredOutputs
{
    SpeakerBase speaker = SpeakerBase::OFF;
    std::optional<Beep> beep;
    RedLedMode redLed = RedLedMode::OFF;
    bool redErrorBlink= false;
    RgbColor rgb= RgbColor::GREEN;
};


}  // namespace vigil   
