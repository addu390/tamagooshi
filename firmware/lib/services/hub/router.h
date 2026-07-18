#pragma once

#include <string>
#include <unordered_map>

#include "codec.h"

namespace tama {

class IMessageHandler {
 public:
  virtual ~IMessageHandler() = default;
  virtual void handle(const Envelope& env) = 0;
};

class MessageRouter {
 public:
  void on(const std::string& type, IMessageHandler& handler);
  bool dispatch(const Envelope& env) const;

 private:
  std::unordered_map<std::string, IMessageHandler*> handlers_;
};

}  // namespace tama
