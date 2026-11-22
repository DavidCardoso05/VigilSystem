#include "vigil/net/MqttClient.hpp"

#include <atomic>
#include <cstdint>
#include <mqtt/async_client.h>

#include "vigil/core/Log.hpp"
#include "vigil/net/Protocol.hpp"
#include "vigil/net/Topics.hpp"

namespace vigil {
namespace {

// CONNACK reason of a failed connect. Paho C 1.3 puts the MQTT 5 reason (e.g. 0x87) in the
// return code of the failure, not always in the reason-code field, so check both.
// Anything below 0x80 = TLS failure, timeout or no CONNACK -> -1.
int reasonOf(const mqtt::token& tok) {
    int r = static_cast<int>(tok.get_reason_code());
    if (r >= 0x80) return r;
    int rc = tok.get_return_code();
    return rc >= 0x80 && rc <= 0xFF ? rc : -1;
}

}  // namespace

struct MqttClient::Impl {
    struct ConnectListener : mqtt::iaction_listener {
        Impl* self = nullptr;
        void on_success(const mqtt::token&) override {
            if (self->cb.connected) self->cb.connected();
        }
        void on_failure(const mqtt::token& tok) override {
            syslog(LOG_DEBUG, "MqttClient: connect failed rc=%d reason=0x%02x", tok.get_return_code(),
                   static_cast<unsigned>(tok.get_reason_code()));
            if (self->cb.connectFailed) self->cb.connectFailed(reasonOf(tok));
        }
    };
    struct PublishListener : mqtt::iaction_listener {
        Impl* self = nullptr;
        static int idOf(const mqtt::token& tok) {
            return static_cast<int>(reinterpret_cast<intptr_t>(tok.get_user_context()));
        }
        void on_success(const mqtt::token& tok) override {
            if (self->cb.delivered) self->cb.delivered(idOf(tok), true, static_cast<int>(tok.get_reason_code()));
        }
        void on_failure(const mqtt::token& tok) override {
            if (self->cb.delivered) self->cb.delivered(idOf(tok), false, static_cast<int>(tok.get_reason_code()));
        }
    };
    struct SubscribeListener : mqtt::iaction_listener {
        void on_success(const mqtt::token&) override {}
        void on_failure(const mqtt::token& tok) override {
            syslog(LOG_ERR, "MqttClient: subscribe failed (reason 0x%02x)",
                   static_cast<unsigned>(tok.get_reason_code()));
        }
    };

    Impl(const BrokerConfig& c, Callbacks callbacks)
        : cfg(c), cb(std::move(callbacks)),
          cli("ssl://" + c.host + ":" + std::to_string(c.port), c.clientId,
              mqtt::create_options(MQTTVERSION_5)) {   // no client-side persistence
        connectListener.self = this;
        publishListener.self = this;
        cli.set_connection_lost_handler([this](const std::string& cause) {
            if (cb.connectionLost) cb.connectionLost(cause.empty() ? "connection lost" : cause);
        });
        cli.set_disconnected_handler([this](const mqtt::properties&, mqtt::ReasonCode rc) {
            if (cb.connectionLost)
                cb.connectionLost("server DISCONNECT reason " + std::to_string(static_cast<int>(rc)));
        });
        cli.set_message_callback([this](mqtt::const_message_ptr m) {
            if (!cb.message) return;
            cb.message(m->get_topic(), m->to_string(), m->is_retained(),
                       m->get_properties().contains(mqtt::property::MESSAGE_EXPIRY_INTERVAL));
        });
    }

    BrokerConfig cfg;
    Callbacks cb;
    std::atomic<int> nextToken{1};
    // Listeners are declared before the client so they outlive it.
    ConnectListener connectListener;
    PublishListener publishListener;
    SubscribeListener subscribeListener;
    mqtt::async_client cli;
};

MqttClient::MqttClient(const BrokerConfig& cfg, Callbacks cb)
    : impl_(std::make_unique<Impl>(cfg, std::move(cb))) {}

MqttClient::~MqttClient() = default;

void MqttClient::connect() {
    const BrokerConfig& c = impl_->cfg;
    auto ssl = mqtt::ssl_options_builder()
                   .trust_store(c.caFile)
                   .enable_server_cert_auth(true)
                   .verify(true)                      // host name check
                   .error_handler([](const std::string& msg) {
                       syslog(LOG_WARNING, "MqttClient: TLS: %s", msg.c_str());
                   })
                   .finalize();
    std::string password = c.readSecret();            // never logged
    auto opts = mqtt::connect_options_builder()
                    .mqtt_version(MQTTVERSION_5)
                    .clean_start(true)
                    .properties({mqtt::property(mqtt::property::SESSION_EXPIRY_INTERVAL, 0)})
                    .keep_alive_interval(std::chrono::seconds(c.keepaliveS))
                    .connect_timeout(std::chrono::seconds(10))
                    .user_name(c.username)
                    .password(password)
                    .will(mqtt::message(topics::CONNECTION, Protocol::kLastWill, topics::QOS, true))
                    .ssl(std::move(ssl))
                    .automatic_reconnect(false)       // NetworkMQTTThread runs its own backoff
                    .finalize();
    for (auto& ch : password) ch = 0;
    impl_->cli.connect(opts, nullptr, impl_->connectListener);
}

int MqttClient::publish(const std::string& topic, const std::string& payload, int qos, bool retain) {
    int id = impl_->nextToken.fetch_add(1);
    if (id <= 0) {                                   // wrapped around
        impl_->nextToken = 2;
        id = 1;
    }
    try {
        auto msg = mqtt::make_message(topic, payload, qos, retain);
        impl_->cli.publish(msg, reinterpret_cast<void*>(static_cast<intptr_t>(id)),
                           impl_->publishListener);
        return id;
    } catch (const mqtt::exception& e) {
        syslog(LOG_WARNING, "MqttClient: publish on %s failed: %s", topic.c_str(), e.what());
        return -1;
    }
}

bool MqttClient::subscribe(const std::string& topic, int qos) {
    try {
        impl_->cli.subscribe(topic, qos, nullptr, impl_->subscribeListener);
        return true;
    } catch (const mqtt::exception& e) {
        syslog(LOG_ERR, "MqttClient: subscribe %s failed: %s", topic.c_str(), e.what());
        return false;
    }
}

bool MqttClient::publishAndWait(const std::string& topic, const std::string& payload, int qos,
                                bool retain, int timeoutMs) {
    try {
        auto tok = impl_->cli.publish(mqtt::make_message(topic, payload, qos, retain));
        return tok->wait_for(std::chrono::milliseconds(timeoutMs)) &&
               static_cast<int>(tok->get_reason_code()) < 0x80;
    } catch (const mqtt::exception& e) {
        syslog(LOG_WARNING, "MqttClient: publish on %s failed: %s", topic.c_str(), e.what());
        return false;
    }
}

void MqttClient::disconnect(int timeoutMs) {
    try {
        // Paho C++ 1.2 does not pass a DISCONNECT reason code to the broker, so a
        // "disconnect with Will" (0x04) is not possible: callers publish the offline message.
        impl_->cli.disconnect(std::chrono::milliseconds(timeoutMs))->wait_for(std::chrono::milliseconds(timeoutMs));
    } catch (const mqtt::exception& e) {
        syslog(LOG_WARNING, "MqttClient: disconnect: %s", e.what());
    }
}

bool MqttClient::isConnected() const { return impl_->cli.is_connected(); }

}  // namespace vigil
