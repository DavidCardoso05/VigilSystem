// Protocol::parseCmd and builders (guide §10.4, §10.6).
#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "vigil/net/Protocol.hpp"

using namespace vigil;
using nlohmann::json;

static std::optional<RemoteCmd> parse(const std::string& p, std::string* why = nullptr) {
    return Protocol::parseCmd(p, /*retained=*/false, /*hasExpiry=*/true, why);
}

TEST(ParseCmd, Arm) {
    auto c = parse(R"({"request_id":"vigil-app-1a2b3c4d-17","type":"arm"})");
    ASSERT_TRUE(c);
    EXPECT_EQ(c->type, RemoteCmd::Type::Arm);
    EXPECT_EQ(c->requestId, "vigil-app-1a2b3c4d-17");
}

TEST(ParseCmd, Disarm) {
    auto c = parse(R"({"request_id":"r1","type":"disarm","pin":"1234"})");
    ASSERT_TRUE(c);
    EXPECT_EQ(c->type, RemoteCmd::Type::Disarm);
    EXPECT_EQ(c->pin, "1234");
}

TEST(ParseCmd, ChangePin) {
    auto c = parse(R"({"request_id":"r1","type":"change_pin","pin":"1234","new_pin":"5678"})");
    ASSERT_TRUE(c);
    EXPECT_EQ(c->type, RemoteCmd::Type::ChangePin);
    EXPECT_EQ(c->newPin, "5678");
}

TEST(ParseCmd, GetEventsSince) {
    auto c = parse(R"({"request_id":"r1","type":"get_events","since":42})");
    ASSERT_TRUE(c);
    EXPECT_EQ(c->type, RemoteCmd::Type::GetEvents);
    EXPECT_EQ(c->since, 42);
    auto d = parse(R"({"request_id":"r1","type":"get_events"})");
    ASSERT_TRUE(d);
    EXPECT_EQ(d->since, 0);              // missing -> 0
}

TEST(ParseCmd, UnknownFieldsIgnored) {
    EXPECT_TRUE(parse(R"({"request_id":"r1","type":"arm","extra":[1,2]})"));
}

TEST(ParseCmd, RetainedOrWithoutExpiryIgnored) {
    const std::string ok = R"({"request_id":"r1","type":"arm"})";
    EXPECT_FALSE(Protocol::parseCmd(ok, true, true));
    EXPECT_FALSE(Protocol::parseCmd(ok, false, false));
}

TEST(ParseCmd, RejectsBadInput) {
    const char* bad[] = {
        "not json",
        "[1,2]",
        R"("arm")",
        R"({"type":"arm"})",                                      // no request_id
        R"({"request_id":5,"type":"arm"})",                       // request_id not a string
        R"({"request_id":"","type":"arm"})",                      // empty
        R"({"request_id":"r1"})",                                 // no type
        R"({"request_id":"r1","type":"ARM"})",                    // unknown type
        R"({"request_id":"r1","type":"reboot"})",
        R"({"request_id":"r1","type":"disarm"})",                 // no pin
        R"({"request_id":"r1","type":"disarm","pin":1234})",      // pin not a string
        R"({"request_id":"r1","type":"change_pin","pin":"1234"})",// no new_pin
        R"({"request_id":"r1","type":"get_events","since":-1})",
        R"({"request_id":"r1","type":"get_events","since":1.5})",
        R"({"request_id":"r1","type":"get_events","since":"3"})",
        R"({"request_id":"r1","type":"get_events","since":true})",
    };
    for (const char* p : bad) EXPECT_FALSE(parse(p)) << p;
    EXPECT_FALSE(parse(R"({"request_id":")" + std::string(65, 'x') + R"(","type":"arm"})"));
    EXPECT_TRUE(parse(R"({"request_id":")" + std::string(64, 'x') + R"(","type":"arm"})"));
}

TEST(ParseCmd, ReasonNeverContainsThePin) {
    std::string why;
    EXPECT_FALSE(parse(R"({"request_id":"r1","type":"bogus","pin":"9876"})", &why));
    EXPECT_FALSE(why.empty());
    EXPECT_EQ(why.find("9876"), std::string::npos);
}

TEST(Payloads, RemoteEvents) {
    RemoteCmd c;
    c.requestId = "r9";
    c.pin = "1234";
    c.newPin = "5678";
    EXPECT_EQ(json::parse(Protocol::remoteArmPayload(c)), json({{"request_id", "r9"}}));
    EXPECT_EQ(json::parse(Protocol::remoteDisarmPayload(c)), json({{"request_id", "r9"}, {"pin", "1234"}}));
    EXPECT_EQ(json::parse(Protocol::remoteChangePinPayload(c)),
              json({{"request_id", "r9"}, {"pin", "1234"}, {"new_pin", "5678"}}));
}

TEST(Payloads, ConnectionAndResponses) {
    EXPECT_EQ(Protocol::connectionOnline(0), R"({"state":"online","timestamp":"1970-01-01T00:00:00Z"})");
    EXPECT_STREQ(Protocol::kLastWill, R"({"state":"offline"})");
    EXPECT_EQ(json::parse(Protocol::response("r1", "OK")), json({{"request_id", "r1"}, {"result", "OK"}}));
    EXPECT_EQ(json::parse(Protocol::response("r1", "LOCKED_OUT", std::string("4"))),
              json({{"request_id", "r1"}, {"result", "LOCKED_OUT"}, {"detail", "4"}}));
    EXPECT_EQ(json::parse(Protocol::getEventsReply("r1", "[]")),
              json({{"request_id", "r1"}, {"result", "OK"}, {"events", json::array()}}));
}
