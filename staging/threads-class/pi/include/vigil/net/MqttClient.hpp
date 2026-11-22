// Wrapper on Eclipse Paho MQTT C++ (mqtt::async_client), MQTT 5 over TLS (guide §7.10, §10.2).
// The callbacks run on Paho's own thread: NetworkMQTTThread only uses them to post NetCmd
// into its own queue, so only that thread touches MQTT state.
#pragma once

#include <functional>
#include <memory>
#include <string>

#include "vigil/net/BrokerConfig.hpp"

namespace vigil {

class MqttClient {
public:
    struct Callbacks {
        std::function<void()> connected;                       // CONNACK 0x00
        std::function<void(int reason)> connectFailed;         // CONNACK reason or -1
        std::function<void(const std::string& cause)> connectionLost;
        std::function<void(const std::string& topic, const std::string& payload,
                           bool retained, bool hasExpiry)> message;
        std::function<void(int token, bool ok, int reason)> delivered;   // PUBACK
    };

    MqttClient(const BrokerConfig& cfg, Callbacks cb);   // throws on an invalid URI
    ~MqttClient();
    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    // Starts an asynchronous connect: client_id, Clean Start 1, session expiry 0,
    // keep-alive, Last Will {"state":"offline"} retained QoS 1, TLS with CA + host name check,
    // automatic_reconnect(false). Result through connected / connectFailed.
    void connect();
    // Returns a local token id (> 0) reported back in `delivered`, or -1 if not sent.
    int  publish(const std::string& topic, const std::string& payload, int qos, bool retain);
    bool subscribe(const std::string& topic, int qos);
    // Publishes and waits for the PUBACK (shutdown paths only). Returns true on PUBACK success.
    bool publishAndWait(const std::string& topic, const std::string& payload, int qos, bool retain,
                        int timeoutMs);
    // Normal DISCONNECT (0x00): the broker discards the Last Will.
    void disconnect(int timeoutMs);
    bool isConnected() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vigil
