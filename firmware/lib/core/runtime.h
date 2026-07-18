#pragma once

#include <cstdint>
#include <optional>

#include "config.h"
#include "expression.h"
#include "gfx.h"
#include "hub/handlers.h"
#include "hub/router.h"
#include "input.h"
#include "input/mapper.h"
#include "model.h"
#include "navigator.h"
#include "prompt_overlay.h"
#include "mascots/registry.h"
#include "channels.h"
#include "system.h"
#include "telemetry.h"

namespace tama {

class Runtime {
 public:
  Runtime(const DeviceCapabilities& caps, const ICodec& codec, IExpressionSink& expression,
          ISystemControl& system, IButtonSource& buttons, IInputSource& input,
          ISensorSource& sensor, ITelemetry& telemetry, IMicSource& mic, config::ISource& config);

  void begin();
  void loop(uint32_t nowMs);
  void bind(const ChannelBinding& binding);

  DeviceState& state() { return state_; }
  Navigator& nav() { return nav_; }
  Gfx& gfx() { return gfx_; }
  MessageRouter& router() { return router_; }
  Channels& channels() { return channels_; }

 private:
  void onGesture(const GestureEvent& event);
  void onMotion(const MotionEvent& event);
  void syncPrompt();
  void syncPower(uint32_t nowMs);
  void renderIfNeeded();

  DeviceState state_;
  PetState pet_;
  Gfx gfx_;
  CharacterRegistry characters_;
  PromptOverlay prompt_;
  std::optional<Page> projected_prompt_;
  MessageRouter router_;
  InputMapper mapper_;
  Navigator nav_;
  HandlerSet handlers_;
  Channels channels_;
  IExpressionSink& expression_;
  ISystemControl& system_;
  IInputSource& gestures_;
  ISensorSource& sensor_;
  ITelemetry& telemetry_;
  IMicSource& mic_;
  config::ISource& config_;
  uint8_t applied_brightness_ = 0;
  uint32_t next_power_poll_ms_ = 0;
};

}  // namespace tama
