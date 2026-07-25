#pragma once

#include <hid.h>

#include "context.h"
#include "theme.h"
#include "widgets.h"

namespace tama {

class HidSession {
 public:
  void enter(ShellContext& ctx) {
    link_ = ctx.hid;
    if (link_) link_->activate();
  }

  void exit() {
    if (link_) link_->deactivate();
    link_ = nullptr;
  }

  bool live() const { return link_ && link_->ready(capability_); }

  void statusPill(Gfx& g, ShellContext& ctx, int cx, int y) const {
    widgets::pill(g, cx, y, statusLabel(ctx), typeface::micro(), statusColor(ctx));
  }

 protected:
  explicit HidSession(HidCapability capability) : capability_(capability) {}

  IHidLink* link() const { return link_; }

 private:
  const char* statusLabel(ShellContext& ctx) const {
    if (!link_) return "UNAVAILABLE";
    if (!ctx.link.enabled()) return "BT OFF";
    if (live()) return "LIVE";
    return ctx.link.connected() ? "CONNECTING" : "PAIR TO USE";
  }

  uint16_t statusColor(ShellContext& ctx) const {
    if (live()) return theme::kHi;
    if (!link_ || !ctx.link.enabled()) return theme::kDim;
    return theme::kWarn;
  }

  IHidLink* link_ = nullptr;
  HidCapability capability_;
};

class GamepadSession : public HidSession {
 public:
  GamepadSession() : HidSession(HidCapability::Gamepad) {}

  void send(const GamepadFrame& frame) {
    if (auto* link = this->link()) link->send(frame);
  }
};

class MediaSession : public HidSession {
 public:
  MediaSession() : HidSession(HidCapability::Media) {}

  void tap(MediaKey key) {
    if (auto* link = this->link()) link->tap(key);
  }
};

class KeyboardSession : public HidSession {
 public:
  KeyboardSession() : HidSession(HidCapability::Keyboard) {}

  void tap(KeyboardKey key) {
    if (auto* link = this->link()) link->tap(key);
  }
};

class MouseSession : public HidSession {
 public:
  MouseSession() : HidSession(HidCapability::Mouse) {}

  void nudge(int8_t dx, int8_t dy) {
    if (auto* link = this->link()) link->nudge(dx, dy);
  }
};

}  // namespace tama
