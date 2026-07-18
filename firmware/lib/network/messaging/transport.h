#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "link.h"

namespace tama {

class IConnection {
 public:
  using MessageHandler =
      std::function<void(const std::string& topic, const std::string& payload)>;
  using ConnectionHandler = std::function<void(bool connected)>;

  virtual ~IConnection() = default;
  virtual void begin() = 0;
  virtual void loop() = 0;
  virtual bool connected() const = 0;
  virtual void onMessage(MessageHandler handler) = 0;
  virtual void onConnection(ConnectionHandler handler) = 0;
};

class ITransport : public IConnection {
 public:
  virtual void publish(const std::string& topic, const std::string& payload, uint8_t qos,
                       bool retain) = 0;
};

class TransportProxy : public ITransport {
 public:
  void bind(ITransport* impl) { impl_ = impl; }
  void begin() override { impl_->begin(); }
  void loop() override { impl_->loop(); }
  void publish(const std::string& topic, const std::string& payload, uint8_t qos,
               bool retain) override {
    impl_->publish(topic, payload, qos, retain);
  }
  bool connected() const override { return impl_ != nullptr && impl_->connected(); }
  void onMessage(MessageHandler handler) override { impl_->onMessage(std::move(handler)); }
  void onConnection(ConnectionHandler handler) override { impl_->onConnection(std::move(handler)); }

 private:
  ITransport* impl_ = nullptr;
};

class ILineSink {
 public:
  virtual ~ILineSink() = default;
  virtual void send(const std::string& line) = 0;
};

class NullLineSink : public ILineSink {
 public:
  void send(const std::string&) override {}
};

}  // namespace tama
