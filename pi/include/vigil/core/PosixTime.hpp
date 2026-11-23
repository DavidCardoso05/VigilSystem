#pragma once
// PosixTime: converts the application's monotonic Time into the timespec used by
// the POSIX calls that take an absolute CLOCK_MONOTONIC deadline

#include <time.h>
#include <chrono>

#include "vigil/core/Time.hpp"

namespace vigil {
    inline timespec toTimespec (Time t){
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch()).count();
        timespec ts{};
        if (ns <= 0) {
            return ts;  // a deadline at or before the epoch is already in the past
        }
        ts.tv_sec = static_cast<time_t>(ns / 1'000'000'000);
        ts.tv_nsec = static_cast<long>(ns % 1'000'000'000);
        return ts;
    }
} //namespace vigil