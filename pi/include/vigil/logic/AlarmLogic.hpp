#pragma once
// AlarmLogic: the only decision maker. It holds every piece of system state and applies
// every alarm rule, so behaviour is deterministic and can be tested on the laptop.
// Owned by AlarmControllerThread; never shared between threads.
//
// Pure: no I/O, no clock reads (the caller injects `now`), no allocation in handle().
// It decides but does not act: each call returns an AlarmActions "receipt" that the
// controller executes (outputs to the driver and RGB lines, commands to CommThread).
//
// Current scope: intrusion only. Fire is always inactive, power is always mains and the
// network state is never changed yet; the fire, power and PIN rules join here later.

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>

#include "vigil/core/CommCommand.hpp"
#include "vigil/core/Event.hpp"
#include "vigil/core/Time.hpp"
#include "vigil/logic/DesiredOutputs.hpp"
#include "vigil/logic/IntrusionFsm.hpp"
#include "vigil/logic/OutputPolicy.hpp"
#include "vigil/logic/States.hpp"

namespace vigil {

// What one handle() call asks the controller to do.
// Fixed capacity, so building it never allocates.
struct AlarmActions {
    DesiredOutputs outputs;              // always filled: the outputs as they must be now
    std::array<CommCommand, 8> comm{};   // valid entries: comm[0] .. comm[commCount - 1]
    std::size_t commCount = 0;

    // Overflow is a programming error: no rule produces more than a few commands.
    void add(const CommCommand& cmd) {
        if (commCount >= comm.size()) {
            throw std::logic_error("AlarmActions: comm capacity exceeded");
        }
        comm[commCount++] = cmd;
    }
};

class AlarmLogic {
public:
    // initial is the state restored at boot from the last ARMED/DISARMED event, so it must
    // be ARMED or DISARMED (otherwise IntrusionFsm throws std::invalid_argument).
    AlarmLogic(IntrusionConfig cfg, IntrusionState initial);

    // The only entry point. Applies any deadline that has expired by `now`, then ev.
    // On a queue timeout the controller passes Event{} (EVT_TICK).
    // RecordEvents come first and the status, if it changed, last, so CommThread stores
    // an event before the status that reflects it. The first call after construction
    // always carries the status, which replaces a stale retained status after a reboot.
    AlarmActions handle(const Event& ev, Time now);

    // Earliest pending deadline, or nullopt if none. The controller wakes at this time
    // or after 1 s, whichever comes first (the 1 s heartbeat limit is not applied here).
    std::optional<Time> nextDeadline() const;

    IntrusionState intrusionState() const { return intrusion_.state(); }

private:
    PublishStatus snapshot() const;                // state as the app sees it
    OutputInputs outputInputs(Feedback fb) const;  // what OutputPolicy needs

    IntrusionFsm intrusion_;
    OutputPolicy policy_;
    NetworkState network_ = NetworkState::NET_LOST;  // lost until the broker accepts us
    std::optional<PublishStatus> lastStatus_;        // empty: nothing published yet
};

}  // namespace vigil
