#pragma once
// IGpioInput: one digital input line (IR sensor, MQ-2 DO, button, keypad row).
// Real implementation: GpioLines (libgpiod). Simulated: SimGpioInput.
// No waitEdge: lines are sampled by periodic threads (DEC-003a).

#include <cstdint>

namespace vigil {

    enum class Level : std::uint8_t { LOW, HIGH };

    class IGpioInput {
    public:
        virtual ~IGpioInput() = default;

        // Current level of the line.
        virtual Level read() = 0;
    };

} // namespace vigil