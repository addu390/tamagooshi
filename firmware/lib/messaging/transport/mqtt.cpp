#include "mqtt.h"
#if defined(TAMA_PROTO_MQTT)

#include <Arduino.h>

#include "topics.h"

namespace tama {

namespace {
constexpr uint32_t kReconnectIntervalMs = 3000;
}

MqttTransport::MqttTransport(IBearer& bearer, std::string host, uint16_t port,
                             std::string clientId, std::string willTopic, std::string willPayload)
    : bearer_(bearer),
      host_(std::move(host)),
      port_(port),
      clientId_(std::move(clientId)),
      willTopic_(std::move(willTopic)),
      willPayload_(std::move(willPayload)) {}

void MqttTransport::begin() {
  client_.setServer(host_.c_str(), port_);
  client_.setClientId(clientId_.c_str());
  client_.setCleanSession(false);
  client_.setWill(willTopic_.c_str(), 1, true,
                  reinterpret_cast<const uint8_t*>(willPayload_.data()), willPayload_.size());

  client_.onConnect([this](bool) {
    client_.subscribe(topics::deviceWildcard(clientId_).c_str(), 1);
    client_.subscribe(topics::fleetWildcard(), 1);
    if (connectionHandler_) connectionHandler_(true);
  });
  client_.onDisconnect([this](espMqttClientTypes::DisconnectReason) {
    if (connectionHandler_) connectionHandler_(false);
  });
  client_.onMessage([this](const espMqttClientTypes::MessageProperties&, const char* topic,
                           const uint8_t* payload, size_t len, size_t index, size_t total) {
    handleMessage(topic, payload, len, index, total);
  });
}

void MqttTransport::loop() {
  if (!bearer_.online()) return;
  client_.loop();
  if (!client_.connected()) {
    const uint32_t now = millis();
    if (now - lastReconnectMs_ >= kReconnectIntervalMs) {
      client_.connect();
      lastReconnectMs_ = now;
    }
  }
}

void MqttTransport::publish(const std::string& topic, const std::string& payload, uint8_t qos,
                            bool retain) {
  client_.publish(topic.c_str(), qos, retain,
                  reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
}

bool MqttTransport::connected() const { return client_.connected(); }

void MqttTransport::onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }

void MqttTransport::onConnection(ConnectionHandler handler) {
  connectionHandler_ = std::move(handler);
}

void MqttTransport::handleMessage(const char* topic, const uint8_t* payload, size_t len,
                                  size_t index, size_t total) {
  if (index == 0) rxBuffer_.clear();
  rxBuffer_.append(reinterpret_cast<const char*>(payload), len);
  if (index + len >= total) {
    if (messageHandler_) messageHandler_(std::string(topic), rxBuffer_);
    rxBuffer_.clear();
  }
}

}  // namespace tama

#endif
