// Reconnect policy of NetworkMQTTThread (guide §7.9, rule 18): 1 -> 2 -> 4 -> ... -> 60 s,
// never stop; CONNACK 0x86 / 0x87 -> every 5 min. Pure, unit-tested.
#pragma once

#include <chrono>

namespace vigil {

class ReconnectBackoff {
public:
    static constexpr std::chrono::seconds kFirst{1};
    static constexpr std::chrono::seconds kMax{60};
    static constexpr std::chrono::seconds kAuthRetry{300};

    // Delay before the next attempt after a failure with this CONNACK reason
    // (-1 = TLS failure / timeout / no CONNACK).
    std::chrono::seconds next(int connackReason);
    void reset() { current_ = std::chrono::seconds(0); }

    static bool isAuthError(int connackReason) { return connackReason == 0x86 || connackReason == 0x87; }

private:
    std::chrono::seconds current_{0};
};

}  // namespace vigil
