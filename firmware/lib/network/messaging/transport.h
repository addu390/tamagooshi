#pragma once

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
  virtual void publish(const std::string& topic, const std::string& payload) = 0;
};

}  // namespace tama
