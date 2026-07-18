#pragma once

#include <string>

#include "codec.h"
#include "transport.h"
#include "model.h"
#include "telemetry.h"

namespace tama {

class AgentCommands {
 public:
  AgentCommands(ILink& identity, ITelemetry& telemetry, const DeviceState& state);

  std::string handle(const std::string& cmd);

 private:
  AgentCodec codec_;
  ILink& identity_;
  ITelemetry& telemetry_;
  const DeviceState& state_;
};

}  // namespace tama
