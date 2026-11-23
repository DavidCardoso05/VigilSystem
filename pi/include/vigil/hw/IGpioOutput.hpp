#pragma once
// IGpioOutput: one digital output line (RGB LED channel through its NPN).
// Real implementation: GpioLines (libgpiod). Simulated: SimGpioOutput.

#include "vigil/hw/IGpioInput.hpp" // Level

namespace vigil {

    class IGpioOutput {
    public:
        virtual ~IGpioOutput() = default;

        virtual void write(Level level) = 0;
    };

} // namespace vigil