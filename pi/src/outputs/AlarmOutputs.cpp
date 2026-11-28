#include "vigil/outputs/AlarmOutputs.hpp"

#include "vigil/outputs/Tones.hpp"

namespace vigil {

AlarmOutputs::AlarmOutputs(IPwm& pwm, IGpioOutput& rgbRed, IGpioOutput& rgbGreen,
                           IGpioOutput& rgbBlue)
    : pwm_(pwm), rgbRed_(rgbRed), rgbGreen_(rgbGreen), rgbBlue_(rgbBlue) {}

void AlarmOutputs::apply(const DesiredOutputs& desired, Time now) {
    const PwmSequence* beep = desired.beep ? &tones::forBeep(*desired.beep) : nullptr;
    update(PwmChannel::SPEAKER, speaker_, static_cast<int>(desired.speaker),
           tones::forSpeaker(desired.speaker), beep, now);

    const PwmSequence* blink = desired.redErrorBlink ? &tones::kLedErrorBlink : nullptr;
    update(PwmChannel::RED_LED, redLed_, static_cast<int>(desired.redLed),
           tones::forRedLed(desired.redLed), blink, now);

    showRgb(desired.rgb);
}

void AlarmOutputs::update(PwmChannel channel, Slot& slot, int base,
                          const PwmSequence& baseSequence, const PwmSequence* oneShot,
                          Time now) {
    if (oneShot != nullptr) {
        // Played over the base. The base is (re)sent when the one-shot is over.
        send(channel, *oneShot);
        slot.oneShotEnd = now + tones::totalDuration(*oneShot);
        slot.applied = base;
        slot.restoreBase = (baseSequence.count != 0);
        return;
    }

    if (base != slot.applied) {
        // The base changed: show it at once (it may cut a running one-shot).
        send(channel, baseSequence);
        slot.applied = base;
        slot.oneShotEnd = Time{};
        slot.restoreBase = false;
        return;
    }

    if (slot.restoreBase && now >= slot.oneShotEnd) {
        send(channel, baseSequence);
        slot.restoreBase = false;
    }
}

void AlarmOutputs::send(PwmChannel channel, const PwmSequence& sequence) {
    if (sequence.count == 0) {
        pwm_.stop(channel);
    } else if (!pwm_.play(channel, sequence)) {
        ++pwmFailures_;
    }
}

void AlarmOutputs::showRgb(RgbColor color) {
    if (rgbShown_ && *rgbShown_ == color) {
        return;
    }
    rgbShown_ = color;

    const bool red = (color == RgbColor::YELLOW || color == RgbColor::MAGENTA);
    const bool green = (color == RgbColor::GREEN || color == RgbColor::YELLOW);
    const bool blue = (color == RgbColor::BLUE || color == RgbColor::MAGENTA);

    rgbRed_.write(red ? Level::HIGH : Level::LOW);
    rgbGreen_.write(green ? Level::HIGH : Level::LOW);
    rgbBlue_.write(blue ? Level::HIGH : Level::LOW);
}

}  // namespace vigil
