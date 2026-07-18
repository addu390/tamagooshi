#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "buddy/controller.h"
#include "buddy/voice/controller.h"
#include "buddy/voice/uplink.h"
#include "codec.h"
#include "commands.h"
#include "inbound.h"
#include "transport.h"

namespace tama {

class AgentSession : public IInboundPipeline {
 public:
  AgentSession(ILineSink& sink, BuddyController& controller, AgentCommands& commands,
               VoiceController& voice, VoiceUplink& uplink);

  void onConnected(bool connected) override;
  void onInbound(const std::string& topic, const std::string& payload) override;
  void tick(uint32_t nowMs) override;

  void decide(const std::string& id, bool allow);
  void decideVoice(const std::string& id, bool send);

 private:
  ILineSink& sink_;
  BuddyController& controller_;
  AgentCommands& commands_;
  VoiceController& voice_;
  VoiceUplink& uplink_;
  AgentCodec codec_;
  uint32_t now_ms_ = 0;
};

std::function<void(const std::string&, PromptOutcome)> makeAgentResolver(AgentSession& session);
std::function<void(const std::string&, PromptOutcome)> makeVoiceResolver(AgentSession& session);

}  // namespace tama
