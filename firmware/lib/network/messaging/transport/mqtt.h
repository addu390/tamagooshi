#pragma once
#if defined(TAMA_PROTO_MQTT)

#include <espMqttClient.h>

#include <cstdint>
#include <string>

#include "bearer.h"
#include "transport.h"

namespace tama {

class MqttTransport : public ITransport {
 public:
  MqttTransport(IBearer& bearer, std::string host, uint16_t port, std::string clientId,
                std::string willTopic, std::string willPayload);

  void begin() override;
  void loop() override;
  void publish(const std::string& topic, const std::string& payload, uint8_t qos,
               bool retain) override;
  bool connected() const override;
  void onMessage(MessageHandler handler) override;
  void onConnection(ConnectionHandler handler) override;

 private:
  void handleMessage(const char* topic, const uint8_t* payload, size_t len, size_t index,
                     size_t total);

  IBearer& bearer_;
  espMqttClient client_;
  std::string host_;
  uint16_t port_;
  std::string clientId_;
  std::string willTopic_;
  std::string willPayload_;
  std::string rxBuffer_;

  MessageHandler messageHandler_;
  ConnectionHandler connectionHandler_;
  uint32_t lastReconnectMs_ = 0;
};

}  // namespace tama

#endif
