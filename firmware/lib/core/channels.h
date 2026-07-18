#pragma once

#include <functional>
#include <string>

#include "inbound.h"
#include "model.h"
#include "transport.h"
#include "wifi/control.h"

namespace tama {

class ICodec;

struct Channel {
  IConnection* connection = nullptr;
  IInboundPipeline* pipeline = nullptr;
  bool valid() const { return connection != nullptr && pipeline != nullptr; }
};

struct ChannelBinding {
  Channel hub;
  Channel agent;
  ILink* link = nullptr;
  IWifiControl* wifi = nullptr;
  IVoiceUplink* voice = nullptr;
  std::function<void(const Page&, PromptOutcome)> resolvePrompt;
};

class Channels {
 public:
  void bind(const ChannelBinding& binding) {
    hub_ = binding.hub;
    agent_ = binding.agent;
  }

  Channel& hub() { return hub_; }
  Channel& agent() { return agent_; }

 private:
  Channel hub_;
  Channel agent_;
};

std::function<void(const std::string&, PromptOutcome)> makeHubResolver(ITransport& transport,
                                                                       const ICodec& codec,
                                                                       std::string deviceId);

}  // namespace tama
