#pragma once

#include <cstddef>

#include "vigil/core/Time.hpp"
#include "vigil/logic/DesiredOutputs.hpp" 
#include "vigil/hw/IPwm.hpp"

namespace vigil::tones {
template <std::size_t N>

constexpr PwmSequence makeSequence (const PwmStep (&steps0)[N], bool repeat){
    static_assert(N <= kMaxPwmSteps, "Too many steps for a PwmSequence");
    PwmSequence sequence{};
    for (std::size_t i = 0; i < N; ++i) {
        sequence.steps[i] = steps0[i];
    }
    sequence.count = N;
    sequence.repeat = repeat;
    return sequence;
}

constexpr Duration totalDuration(const PwmSequence& sequence){
    Duration total{0};
    for (std::size_t i=0; i<sequence.count; i++){
        total += Duration{sequence.steps[i].duration_ms};
    }
    return total;
}
   
inline constexpr PwmStep kEntryWarningSteps[] = {{1000, 50, 200}, {0, 0, 800}};
inline constexpr PwmStep kIntrusionSteps[]    = {{1500, 50, 300}, {900, 50, 300}};
inline constexpr PwmStep kFireSteps[]         = {{2800, 50, 150}, {0, 0, 150}};

inline constexpr PwmStep kKeyBeepSteps[]   = {{2000, 50, 50}};
inline constexpr PwmStep kErrorBeepSteps[] = {{400, 50, 120}, {0, 0, 60}, {400, 50, 120}};

inline constexpr PwmStep kLedOnSteps[]    = {{1000, 100, 1000}};
inline constexpr PwmStep kLedFlashSteps[] = {{1000, 100, 250}, {0, 0, 250}};
  
inline constexpr PwmStep kLedErrorBlinkSteps[] = {
    {1000, 100, 100}, {0, 0, 100}, {1000, 100, 100}, {0, 0, 100}, {1000, 100, 100}, {0, 0, 100}};

inline constexpr PwmSequence kEntryWarning = makeSequence(kEntryWarningSteps, true);
inline constexpr PwmSequence kIntrusion    = makeSequence(kIntrusionSteps, true);
inline constexpr PwmSequence kFire         = makeSequence(kFireSteps, true);
inline constexpr PwmSequence kKeyBeep      = makeSequence(kKeyBeepSteps, false);
inline constexpr PwmSequence kErrorBeep    = makeSequence(kErrorBeepSteps, false);
inline constexpr PwmSequence kLedOn        = makeSequence(kLedOnSteps, true);
inline constexpr PwmSequence kLedFlash     = makeSequence(kLedFlashSteps, true);
inline constexpr PwmSequence kLedErrorBlink = makeSequence(kLedErrorBlinkSteps, false);

inline constexpr PwmSequence kSilence{};

inline constexpr const PwmSequence& forSpeaker(SpeakerBase base) {
    switch (base) {
        case SpeakerBase::ENTRY_WARNING: return kEntryWarning;
        case SpeakerBase::INTRUSION:     return kIntrusion;
        case SpeakerBase::FIRE:          return kFire;
        case SpeakerBase::OFF:           break;
    }
    return kSilence;
}

inline constexpr const PwmSequence& forBeep(Beep beep) {
    return beep == Beep::KEY ? kKeyBeep : kErrorBeep;
}

inline constexpr const PwmSequence& forRedLed(RedLedMode mode) {
    switch (mode) {
        case RedLedMode::ON:       return kLedOn;
        case RedLedMode::FLASHING: return kLedFlash;
        case RedLedMode::OFF:      break;
    }
    return kSilence;
}

constexpr Duration beepDuration(Beep beep) { return totalDuration(forBeep(beep)); }
constexpr Duration errorBlinkDuration() { return totalDuration(kLedErrorBlink); }

}  // namespace vigil::tones


