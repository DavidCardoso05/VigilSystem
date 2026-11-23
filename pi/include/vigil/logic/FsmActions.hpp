#pragma once
// FsmActions: what one FSM step asks the caller to do.
// Fixed capacity, so building it never allocates.

#include <array>
#include <cstddef>
#include <stdexcept>

#include "vigil/core/CommCommand.hpp"
#include "vigil/logic/States.hpp"

namespace vigil {

struct FsmActions {
    Feedback feedback = Feedback::NONE;
    std::array<CommCommand, 8> comm{};
    std::size_t commCount = 0;

    // Overflow is a programming error (no step produces more than a few commands).
    void add(const CommCommand& cmd) {
        if (commCount >= comm.size()) {
            throw std::logic_error("FsmActions: comm capacity exceeded");
        }
        comm[commCount++] = cmd;
    }
};

}  // namespace vigil
