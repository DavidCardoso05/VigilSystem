// Builds and validates every MQTT payload (guide §10.4, §10.6). Pure: no I/O, unit-tested.
#pragma once

#include <ctime>
#include <optional>
#include <string>

namespace vigil {

struct RemoteCmd {
    enum class Type { Arm, Disarm, ChangePin, GetEvents } type = Type::Arm;
    std::string requestId;
    std::string pin;       // Disarm, ChangePin (secret)
    std::string newPin;    // ChangePin (secret)
    long long   since = 0; // GetEvents
};

namespace Protocol {

// §10.6: ignores retained messages and messages without Message Expiry; requires a JSON
// object with request_id (string, 1-64 chars) and type in arm|disarm|change_pin|get_events.
// On rejection returns nullopt and puts a short reason in *why (never contains the PIN).
std::optional<RemoteCmd> parseCmd(const std::string& payload, bool retained, bool hasExpiry,
                                  std::string* why = nullptr);

// Event Queue payloads (guide §5.4).
std::string remoteArmPayload(const RemoteCmd& c);        // {"request_id"}
std::string remoteDisarmPayload(const RemoteCmd& c);     // {"request_id","pin"}
std::string remoteChangePinPayload(const RemoteCmd& c);  // {"request_id","pin","new_pin"}

// Outgoing messages.
std::string connectionOnline(time_t now);                // {"state":"online","timestamp":...}
constexpr const char* kLastWill = R"({"state":"offline"})";
std::string response(const std::string& requestId, const std::string& result,
                     const std::optional<std::string>& detail = std::nullopt);
// get_events reply; `eventsJsonArray` is a JSON array text ("[]" until StorageThread exists).
std::string getEventsReply(const std::string& requestId, const std::string& eventsJsonArray);

}  // namespace Protocol
}  // namespace vigil
