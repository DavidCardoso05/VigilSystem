#pragma once
// PosixTime: converts the application's monotonic Time into the timespec used by the
// POSIX calls that take an absolute CLOCK_MONOTONIC deadline:
//   - pthread_cond_timedwait with a CLOCK_MONOTONIC condattr (BlockingQueue::popUntil);
//   - clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...) (periodic threads).
//
// Kept apart from Time.hpp so the logic package never includes an OS header.
//
// Assumption, checked by a test: std::chrono::steady_clock is CLOCK_MONOTONIC.
// True for libstdc++ and libc++ on Linux, the only targets (laptop and Pi).

#include <time.h>

#include <chrono>

#include "vigil/core/Time.hpp"

namespace vigil {

inline timespec toTimespec(Time t) {
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
    timespec ts{};
    if (ns <= 0) {
        return ts;  // a deadline at or before the epoch is already in the past
    }
    ts.tv_sec = static_cast<time_t>(ns / 1'000'000'000);
    ts.tv_nsec = static_cast<long>(ns % 1'000'000'000);
    return ts;
}

}  // namespace vigil
