#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <cstdint>
#include <optional>
#include <string>

#include "ble/setup.h"
#include "clock.h"
#include "model.h"
#include "muse/chat.h"
#include "system.h"
#include "transport/noise.h"
#include "wifi/control.h"

namespace tama {

class MuseAgent : public IAgent {
 public:
  MuseAgent(DeviceState& state, IAccountRepository& accounts, IWifiControl& wifi,
            SetupEndpoint& setup, NoiseTransport& link, MuseChat& chat, ISystemControl& system,
            IClockRepository& clock);

  void begin();
  void loop(uint32_t nowMs);
  bool resolve(const Page& page, PromptOutcome outcome);

  AgentState state() const override;
  std::string name() const override { return link_.agent(); }
  std::string beacon() const override { return setup_.name(); }
  void pair() override;
  void cancel() override;
  void unpair() override;

 private:
  void provision(const muse::Provision& provision);
  void join(uint32_t nowMs);
  void sync();
  void connect();
  void watch();

  DeviceState& state_;
  IAccountRepository& accounts_;
  IWifiControl& wifi_;
  SetupEndpoint& setup_;
  NoiseTransport& link_;
  MuseChat& chat_;
  ISystemControl& system_;
  IClockRepository& clock_;

  std::optional<muse::Provision> joining_;
  uint32_t joinedAt_ = 0;
  uint32_t closeAt_ = 0;
  uint32_t now_ = 0;
  bool paired_ = false;
  bool linked_ = false;
  bool timeRequested_ = false;
  bool timeSynced_ = false;
  AgentState shown_ = AgentState::Unpaired;
};

}  // namespace tama

#endif
