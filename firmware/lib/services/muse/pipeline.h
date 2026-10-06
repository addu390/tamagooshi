#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "hub/router.h"
#include "inbound.h"
#include "model.h"
#include "muse/commands.h"
#include "transport.h"

namespace tama {

class MusePipeline : public IInboundPipeline {
 public:
  MusePipeline(ITransport& transport, DeviceState& state, MessageRouter& router);

  void onConnected(bool connected) override;
  void onInbound(const std::string& id, const std::string& payload) override;
  void tick(uint32_t nowMs) override;
  void invoke(const std::string& id, const muse::Invoke& invoke);

  bool resolve(const Page& page, PromptOutcome outcome);

 private:
  struct PendingAlert {
    std::string request;
    std::string page;
    uint32_t since = 0;
  };

  void run(const std::string& id, muse::Verb verb, const std::string& params);
  void route(const char* type, const std::string& body);
  void alert(const std::string& id, const std::string& params);
  void say(const std::string& params);
  void settle(const char* outcome);
  void reply(const std::string& id, const std::string& result);

  ITransport& transport_;
  DeviceState& state_;
  MessageRouter& router_;
  std::optional<PendingAlert> alert_;
  uint32_t now_ = 0;
};

}  // namespace tama
