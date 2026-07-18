#pragma once

#include <cstdint>
#include <string>

#include "buddy/controller.h"
#include "codec.h"
#include "commands.h"
#include "source.h"
#include "transport.h"

namespace tama {

class ClaudeSession : public IInboundPipeline {
 public:
  ClaudeSession(ILineSink& sink, BuddyController& controller, ClaudeCommands& commands);

  void onConnected(bool connected) override;
  void onInbound(const std::string& topic, const std::string& payload) override;
  void tick(uint32_t nowMs) override;

  void decide(const std::string& id, bool allow);

 private:
  ILineSink& sink_;
  BuddyController& controller_;
  ClaudeCommands& commands_;
  ClaudeCodec codec_;
  uint32_t now_ms_ = 0;
};

}  // namespace tama
