// MQTT topics (guide §10.3). All QoS 1.
#pragma once

namespace vigil::topics {

constexpr const char* STATUS     = "vigil/home1/status";      // retained
constexpr const char* CONNECTION = "vigil/home1/connection";  // retained (online / offline / Last Will)
constexpr const char* ALERT      = "vigil/home1/alert";       // outbox
constexpr const char* EVENT      = "vigil/home1/event";
constexpr const char* RESPONSE   = "vigil/home1/response";
constexpr const char* CMD        = "vigil/home1/cmd";         // app -> Pi, never retained, expiry 30 s

constexpr int QOS = 1;

}  // namespace vigil::topics
