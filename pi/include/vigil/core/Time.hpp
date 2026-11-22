#pragma once //copy this file once per compilation

// Monotonic time used across the application.
// Logic classes never read a clock: the caller injects `now`.

#include <chrono>

namespace vigil {
    using Clock = std::chrono::steady_clock; //monoatomic clock

    using Time = Clock::time_point;
    using Duration = std::chrono::milliseconds;
} //namespace vigil