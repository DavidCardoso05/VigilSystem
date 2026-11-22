#include "vigil/net/ReconnectBackoff.hpp"

#include <algorithm>

namespace vigil {

constexpr std::chrono::seconds ReconnectBackoff::kFirst;
constexpr std::chrono::seconds ReconnectBackoff::kMax;
constexpr std::chrono::seconds ReconnectBackoff::kAuthRetry;

std::chrono::seconds ReconnectBackoff::next(int connackReason) {
    if (isAuthError(connackReason)) return kAuthRetry;   // 0x86 / 0x87: every 5 min
    current_ = current_.count() == 0 ? kFirst : std::min(current_ * 2, kMax);
    return current_;                                     // 1, 2, 4, ... 32, 60, 60, ...
}

}  // namespace vigil
