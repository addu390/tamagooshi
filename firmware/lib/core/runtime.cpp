#include "runtime.h"

#include <cstring>

#include "brand.gen.h"
#include "screens.h"

namespace tama {

namespace {

bool sameLabel(const char* lhs, const char* rhs) {
  if (!lhs || !rhs) return lhs == rhs;
  return std::strcmp(lhs, rhs) == 0;
}

bool samePage(const Page& lhs, const Page& rhs) {
  if (lhs.id != rhs.id || lhs.severity != rhs.severity || lhs.title != rhs.title ||
      lhs.body != rhs.body || lhs.source != rhs.source || lhs.ts != rhs.ts ||
      lhs.requires_ack != rhs.requires_ack || lhs.actionCount != rhs.actionCount) {
    return false;
  }
  for (int i = 0; i < lhs.actionCount; ++i) {
    if (!sameLabel(lhs.actions[i].label, rhs.actions[i].label) ||
        lhs.actions[i].outcome != rhs.actions[i].outcome) {
      return false;
    }
  }
  return true;
}

}

Runtime::Runtime(const DeviceCapabilities& caps, const ICodec& codec, IExpressionSink& expression,
                 ISystemControl& system, IButtonSource& buttons, IInputSource& input,
                 ISensorSource& sensor, ITelemetry& telemetry, IMicSource& mic,
                 config::ISource& config)
    : mapper_(buttons, input, caps),
      nav_(state_, pet_, caps, characters_),
      handlers_(state_, codec, expression, system),
      expression_(expression),
      system_(system),
      gestures_(input),
      sensor_(sensor),
      telemetry_(telemetry),
      mic_(mic),
      config_(config) {}

void Runtime::bind(const ChannelBinding& binding) {
  channels_.bind(binding);
  if (binding.link) nav_.setLink(*binding.link);
  nav_.setWifi(binding.wifi);
  nav_.setVoice(binding.voice);
  nav_.setResolver(binding.resolvePrompt);
}

void Runtime::begin() {
  brand::apply(state_);
  config::apply(config_.read(), state_);
  screens::install(nav_, characters_, prompt_);
  nav_.setMic(mic_);
  nav_.setSensor(sensor_);
  handlers_.bind(router_);

  auto wire = [this](Channel& ch, bool isHub) {
    if (!ch.valid()) return;
    IInboundPipeline* pipeline = ch.pipeline;
    ch.connection->onMessage([this, pipeline](const std::string& t, const std::string& p) {
      pipeline->onInbound(t, p);
      renderIfNeeded();
    });
    ch.connection->onConnection([this, pipeline, isHub](bool connected) {
      pipeline->onConnected(connected);
      if (isHub)
        state_.hub_connected = connected;
      else
        state_.agent_connected = connected;
      state_.connected = state_.hub_connected || state_.agent_connected;
      nav_.markDirty();
      renderIfNeeded();
    });
  };
  wire(channels_.hub(), true);
  wire(channels_.agent(), false);

  mapper_.onIntent([this](Intent i) {
    nav_.dispatch(i);
    renderIfNeeded();
  });
  gestures_.onGesture([this](const GestureEvent& e) { onGesture(e); });
  sensor_.onMotion([this](const MotionEvent& e) { onMotion(e); });

  expression_.begin();
  mapper_.begin();
  sensor_.begin();
  gfx_.begin();
  if (channels_.hub().valid()) channels_.hub().connection->begin();
  if (channels_.agent().valid()) channels_.agent().connection->begin();
  system_.setBrightness(state_.brightness);
  applied_brightness_ = state_.brightness;
  syncPower(0);
  nav_.start("boot");
  renderIfNeeded();
}

void Runtime::loop(uint32_t nowMs) {
  if (channels_.hub().valid()) {
    channels_.hub().connection->loop();
    channels_.hub().pipeline->tick(nowMs);
  }
  if (channels_.agent().valid()) {
    channels_.agent().connection->loop();
    channels_.agent().pipeline->tick(nowMs);
  }
  mapper_.poll();
  sensor_.poll();
  syncPrompt();
  syncPower(nowMs);
  nav_.tick(nowMs);
  renderIfNeeded();
}

void Runtime::onGesture(const GestureEvent& event) {
  if (channels_.hub().valid()) channels_.hub().pipeline->onGesture(event);
  if (channels_.agent().valid()) channels_.agent().pipeline->onGesture(event);
}

void Runtime::onMotion(const MotionEvent& event) {
  if (channels_.hub().valid()) channels_.hub().pipeline->onMotion(event);
  if (channels_.agent().valid()) channels_.agent().pipeline->onMotion(event);
}

void Runtime::syncPower(uint32_t nowMs) {
  if (state_.brightness != applied_brightness_) {
    system_.setBrightness(state_.brightness);
    applied_brightness_ = state_.brightness;
  }

  if (nowMs < next_power_poll_ms_) return;
  next_power_poll_ms_ = nowMs + 5000;

  const Telemetry t = telemetry_.read();
  state_.free_heap = t.heap;
  state_.up_secs = t.up_secs;
  if (t.batt_pct != state_.batt_pct || t.usb != state_.charging) {
    state_.batt_pct = t.batt_pct;
    state_.charging = t.usb;
    nav_.markDirty();
  }
}

void Runtime::syncPrompt() {
  if (state_.prompt) {
    if (projected_prompt_ && samePage(*projected_prompt_, *state_.prompt)) return;
    nav_.raisePrompt(*state_.prompt);
    projected_prompt_ = state_.prompt;
    return;
  }
  if (!projected_prompt_) return;
  nav_.clearPrompt(projected_prompt_->id);
  projected_prompt_.reset();
}

void Runtime::renderIfNeeded() {
  if (state_.dirty) {
    state_.dirty = false;
    nav_.markDirty();
  }
  if (!nav_.consumeDirty()) return;
  nav_.render(gfx_);
  expression_.apply(nav_.expressionState());
}

}  // namespace tama
