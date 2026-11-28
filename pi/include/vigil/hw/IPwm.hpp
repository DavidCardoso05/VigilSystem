#pragma once
// IPwm: the two PWM channels played by the vigil_pwm driver (DEC-002, DEC-012 12.4).
// Real implementation: PwmDevice (/dev/vigil_pwm). Simulated: SimPwm.
//
// The kernel plays a sequence of up to 8 steps {frequency, duty, duration} with an
// optional repeat flag. Frequency 0 = silent step. A sequence with 0 steps stops the
// channel and leaves the output LOW. The exact UAPI struct is still open (N-4); this
// header is the user-space view of it.

#include <array>
#include <cstddef>
#include <cstdint>

namespace vigil {

enum class PwmChannel : std::uint8_t {
    SPEAKER,  // PWM0, GPIO18
    RED_LED   // PWM1, GPIO13
};

inline constexpr std::size_t kMaxPwmSteps = 8;
inline constexpr std::uint8_t kMaxSpeakerDutyPercent = 50; // speaker protection

struct PwmStep {
    std::uint32_t frequency_hz = 0;   // 0 = silent step
    std::uint8_t  duty_percent = 0;   // 0..100 (speaker: <= 50)
    std::uint32_t duration_ms  = 0;
};

// Fixed size: no dynamic allocation in the real-time loops.
struct PwmSequence {
    std::array<PwmStep, kMaxPwmSteps> steps{};
    std::size_t count = 0;   // 0 = stop the channel
    bool repeat = false;
};

class IPwm {
public:
    virtual ~IPwm() = default;

    // Replace whatever the channel was playing. Returns false if the sequence was
    // rejected (the driver answers EINVAL/EIO); the channel is then unchanged.
    virtual bool play(PwmChannel channel, const PwmSequence& sequence) = 0;

    // Stop the channel: output LOW.
    virtual void stop(PwmChannel channel) = 0;
};

}  // namespace vigil
