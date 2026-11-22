#include "vigil/net/Protocol.hpp"

#include <climits>
#include <nlohmann/json.hpp>

#include "vigil/core/TimeUtil.hpp"

namespace vigil::Protocol {

using nlohmann::json;

namespace {
bool fail(std::string* why, const char* reason) {
    if (why) *why = reason;
    return false;
}
}  // namespace

std::optional<RemoteCmd> parseCmd(const std::string& payload, bool retained, bool hasExpiry,
                                  std::string* why) {
    if (retained) return fail(why, "retained cmd"), std::nullopt;
    if (!hasExpiry) return fail(why, "cmd without Message Expiry"), std::nullopt;

    json j = json::parse(payload, nullptr, /*allow_exceptions=*/false);
    if (j.is_discarded()) return fail(why, "malformed JSON"), std::nullopt;
    if (!j.is_object()) return fail(why, "not a JSON object"), std::nullopt;

    auto rid = j.find("request_id");
    if (rid == j.end() || !rid->is_string()) return fail(why, "missing request_id"), std::nullopt;
    const std::string& requestId = rid->get_ref<const std::string&>();
    if (requestId.empty() || requestId.size() > 64) return fail(why, "bad request_id length"), std::nullopt;

    auto type = j.find("type");
    if (type == j.end() || !type->is_string()) return fail(why, "missing type"), std::nullopt;
    const std::string& t = type->get_ref<const std::string&>();

    RemoteCmd c;
    c.requestId = requestId;
    auto str = [&](const char* key, std::string& out) {
        auto it = j.find(key);
        if (it == j.end() || !it->is_string()) return false;
        out = it->get<std::string>();
        return true;
    };

    if (t == "arm") {
        c.type = RemoteCmd::Type::Arm;
    } else if (t == "disarm") {
        c.type = RemoteCmd::Type::Disarm;
        if (!str("pin", c.pin)) return fail(why, "disarm without pin"), std::nullopt;
    } else if (t == "change_pin") {
        c.type = RemoteCmd::Type::ChangePin;
        if (!str("pin", c.pin) || !str("new_pin", c.newPin))
            return fail(why, "change_pin without pin/new_pin"), std::nullopt;
    } else if (t == "get_events") {
        c.type = RemoteCmd::Type::GetEvents;
        auto s = j.find("since");
        if (s != j.end()) {
            if (s->is_number_unsigned()) {
                if (s->get<unsigned long long>() > static_cast<unsigned long long>(LLONG_MAX))
                    return fail(why, "since out of range"), std::nullopt;
                c.since = static_cast<long long>(s->get<unsigned long long>());
            } else if (s->is_number_integer()) {
                c.since = s->get<long long>();
                if (c.since < 0) return fail(why, "negative since"), std::nullopt;
            } else {
                return fail(why, "since is not an integer"), std::nullopt;
            }
        }
    } else {
        return fail(why, "unknown type"), std::nullopt;
    }
    return c;
}

std::string remoteArmPayload(const RemoteCmd& c) {
    return json{{"request_id", c.requestId}}.dump();
}

std::string remoteDisarmPayload(const RemoteCmd& c) {
    return json{{"request_id", c.requestId}, {"pin", c.pin}}.dump();
}

std::string remoteChangePinPayload(const RemoteCmd& c) {
    return json{{"request_id", c.requestId}, {"pin", c.pin}, {"new_pin", c.newPin}}.dump();
}

std::string connectionOnline(time_t now) {
    return json{{"state", "online"}, {"timestamp", isoUtc(now)}}.dump();
}

std::string response(const std::string& requestId, const std::string& result,
                     const std::optional<std::string>& detail) {
    json j{{"request_id", requestId}, {"result", result}};
    if (detail) j["detail"] = *detail;
    return j.dump();
}

std::string getEventsReply(const std::string& requestId, const std::string& eventsJsonArray) {
    json events = json::parse(eventsJsonArray);
    if (!events.is_array()) events = json::array();
    return json{{"request_id", requestId}, {"result", "OK"}, {"events", events}}.dump();
}

}  // namespace vigil::Protocol
