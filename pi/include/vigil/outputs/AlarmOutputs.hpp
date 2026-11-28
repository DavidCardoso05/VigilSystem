#pragma once 

#include <cstddef>
#include <optional>

#include "vigil/logic/DesiredOutputs.hpp"
#include "vigil/core/Time.hpp"
#include "vigil/hw/IGpioOutput.hpp"
#include "vigil/hw/IPwm.hpp"

namespace vigil {

class AlarmOutputs {
    
public:
AlarmOutputs(IPwm & pwm, IGpioOutput & rgbRed, IGpioOutput & rgbGreen, IGpioOutput & rgbBlue);

void apply(const DesiredOutputs & desires, Time now);

 std::size_t pwmFailures() const { return pwmFailures_; }

private:
        
    struct Slot {
        int applied = 0;         
        Time oneShotEnd{};        
        bool restoreBase = false; 
    };

    void update(PwmChannel channel, Slot& slot, int base, const PwmSequence& baseSequence,
                const PwmSequence* oneShot, Time now);
    void send(PwmChannel channel, const PwmSequence& sequence);
    void showRgb(RgbColor color);

    IPwm& pwm_;
    IGpioOutput& rgbRed_;
    IGpioOutput& rgbGreen_;
    IGpioOutput& rgbBlue_;

    Slot speaker_;
    Slot redLed_;
    std::optional<RgbColor> rgbShown_;
    std::size_t pwmFailures_ = 0;
};


}  // namespace vigil
