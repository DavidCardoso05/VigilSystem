#include "vigil/logic/IntrusionFsm.hpp"

#include <stdexcept>

namespace vigil {

    namespace{
        EventLogDetail detailOf(EventSource src){
            switch (src){
                case EventSource::IR1:    return EventLogDetail::IR1;
                case EventSource::IR2:    return EventLogDetail::IR2;
                case EventSource::LOCAL: return EventLogDetail::LOCAL;
                case EventSource::REMOTE: return EventLogDetail::REMOTE;
                case EventSource::NONE: break;
            }
            return EventLogDetail::NONE;
        }


    } //namespace

    IntrusionFsm::IntrusionFsm(IntrusionConfig cfg, IntrusionState initial)
    : cfg_(cfg), state_(initial){
        if(initial != IntrusionState::ARMED && initial !=IntrusionState::DISARMED){
            throw std::invalid_argument("IntrusionFsm: initial state must be ARMED or DISARMED");
        }
    }

    FsmActions IntrusionFsm::onEvent(const Event& ev, Time now, bool fireAlarmActive){
    FsmActions out;
    applyDeadline(out, now, fireAlarmActive);
    handle(ev, out, now, fireAlarmActive);
    return out;
    }

    void IntrusionFsm::applyDeadline(FsmActions& out, Time now, bool fireAlarmActive){
        if (!deadline_ || now < *deadline_) {
            return;
        }
        deadline_.reset();

        switch (state_){
            case IntrusionState::EXIT_DELAY:
                if(fireAlarmActive){
                    //Stay; the keypad PIN disables the fire alarm
                    return;
                }
                state_= IntrusionState::ARMED;
                out.add(RecordEvent{EventLogType::ARMED, EventLogDetail::LOCAL});
                return;
            
            case IntrusionState::ENTRY_DELAY: //dont care about fire active
                state_ = IntrusionState::INTRUSION_ALARM;
                out.add(RecordEvent{EventLogType::ARMED, motionSensor_});
                return;
            
            case IntrusionState::ARMED:
            case IntrusionState::DISARMED:
            case IntrusionState::INTRUSION_ALARM:
                return; // no deadline is ever held in these states    
        }
    }

    void IntrusionFsm::handle(const Event& ev, FsmActions& out, Time now, bool fireAlarmActive){
        switch(ev.type){
            case EventType::EVT_TICK: 
                return;
            
            case EventType::EVT_BUTTON_PRESS:
                if(state_ == IntrusionState::DISARMED && !fireAlarmActive){
                    state_ == IntrusionState::EXIT_DELAY;
                    deadline_ = now + cfg_.exitDelay;
                    out.feedback = Feedback::KEY_BEEP;
                } else {
                    out.feedback = Feedback::ERROR_BEEP;
                }
            
            case EventType::EVT_REMOTE_ARM:
                if ((state_ == IntrusionState::DISARMED || state_ == IntrusionState::ENTRY_DELAY)
                 && !fireAlarmActive){
                    state_ = IntrusionState::ARMED;
                    deadline_.reset();
                    out.add(RecordEvent{EventLogType::ARMED, EventLogDetail::REMOTE});
                }
                return; // replies belong to alarm logic
            
            case EventType::EVT_MOTION_DETECTED:
                if (state_ == IntrusionState::ARMED &&
                (ev.source == EventSource::IR1 || ev.source == EventSource::IR2)){
                    state_ = IntrusionState::ENTRY_DELAY;
                    deadline_ = now + cfg_.entryDelay;
                    motionSensor_ = detailOf(ev.source);
                }
                return;
            
            case EventType::EVT_PIN_CORRECT:
                if(state_ != IntrusionState::DISARMED && 
                (ev.source == EventSource::LOCAL || ev.source == EventSource::REMOTE)){
                    state_ = IntrusionState::DISARMED;
                    deadline_.reset();
                    motionSensor_ = EventLogDetail::NONE;
                    out.add(RecordEvent{EventLogType::DISARMED, detailOf(ev.source)});
                }
                return;
        }
    }



} //namespace vigil