#pragma once

#include <string>

namespace tama {

enum class AgentState { Unpaired, Pairing, Confirming, Joining, Connecting, Online };

class IAgent {
 public:
  virtual ~IAgent() = default;
  virtual AgentState state() const = 0;
  virtual std::string name() const = 0;
  virtual std::string beacon() const = 0;
  virtual void pair() = 0;
  virtual void cancel() = 0;
  virtual void unpair() = 0;
};

}  // namespace tama
