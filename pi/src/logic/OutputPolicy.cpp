#include "vigil/logic/OutputPolicy.hpp"

namespace vigil {

    namespace{
        SpeakerBase speakerBase(const OutputInputs& in) {
            if (in.fire == FireState::FIRE_ALARM) {
                return SpeakerBase::FIRE;
            }
            if (in.intrusion == IntrusionState::INTRUSION_ALARM) {
                return SpeakerBase::INTRUSION;
            }
            if (in.intrusion == IntrusionState::EXIT_DELAY) {
                return SpeakerBase::ENTRY_WARNING;
            }
            return SpeakerBase::OFF;
        }
    

        RedLedMode redLed(const OutputInputs& in) {
            if(in.fire == FireState::FIRE_ALARM || in.intrusion == IntrusionState::INTRUSION_ALARM ) {
                return RedLedMode::FLASHING;
            }   
            switch (in.intrusion){
                case IntrusionState::EXIT_DELAY:
                case IntrusionState::ARMED:
                case IntrusionState::ENTRY_DELAY:
                    return RedLedMode::ON;
                default:
                    return RedLedMode::OFF;
            }
        }   

        RgbColor rgb(const OutputInputs& in) {
            const bool connected = in.network == NetworkState::NET_CONNECTED;
            if (in.power ==PowerSource::GRID){
                return connected ? RgbColor::GREEN : RgbColor::YELLOW;        
            }
            return connected ? RgbColor::BLUE : RgbColor::MAGENTA;     
        }

    }// namespace

    DesiredOutputs OutputPolicy::compute(const OutputInputs& in)const{

        DesiredOutputs out; 
        out.speaker = speakerBase(in);
        out.redLed = redLed(in);
        out.rgb = rgb(in);

        const bool alarmSounding = out.speaker == SpeakerBase::FIRE || out.speaker == SpeakerBase::INTRUSION;

        if(in.beep && !alarmSounding){
            out.beep = in.beep;
        }
        out.redErrorBlink = in.wrongPinBlink && out.redLed !=RedLedMode::FLASHING;
        return out;
    }


} // namespace vigil




