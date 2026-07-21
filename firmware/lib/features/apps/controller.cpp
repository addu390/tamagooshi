#include "brand.gen.h"
#if TAMA_APP_CONTROLLER

#include <algorithm>
#include <cmath>

#include "apps.h"
#include "motion.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr float kSmoothing = 0.4f;
constexpr float kFullTilt = 0.55f;
constexpr float kDeadzone = 0.04f;
constexpr uint32_t kSampleMs = 20;
constexpr uint32_t kRedrawMs = 100;

int8_t toAxis(float tilt) {
  if (std::fabs(tilt) < kDeadzone) return 0;
  const float scaled = std::clamp(tilt / kFullTilt, -1.0f, 1.0f);
  return static_cast<int8_t>(std::lround(scaled * 127.0f));
}

class ControllerScreen : public AppScreen {
 public:
  const char* id() const override { return "app.controller"; }

  void onEnter(ShellContext& ctx) override {
    gamepad_ = ctx.gamepad;
    filter_.reset();
    frame_ = GamepadFrame{};
    if (gamepad_) gamepad_->activate();
  }

  void onExit() override {
    if (gamepad_) gamepad_->deactivate();
    gamepad_ = nullptr;
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (!sampler_.due(nowMs, kSampleMs)) return Transition::none();

    if (filter_.sample(&ctx.sensor)) {
      frame_.x = toAxis(filter_.x());
      frame_.y = toAxis(-filter_.y());
    }
    frame_.buttons = static_cast<uint8_t>((ctx.buttons.held(0) ? 0x01 : 0) |
                                          (ctx.buttons.held(1) ? 0x02 : 0));
    if (gamepad_) gamepad_->send(frame_);

    return redraw_.due(nowMs, kRedrawMs) ? Transition::redraw() : Transition::none();
  }

  Transition handleInput(Intent, ShellContext&) override { return Transition::none(); }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "GAMEPAD");
    const bool live = gamepad_ && gamepad_->ready();
    const char* status = statusLabel(ctx, live);
    const uint16_t accent = statusColor(ctx, live);

    if (L.landscape) {
      const int contentTop = L.top + 6;
      const int pad = L.bottom - contentTop - 6;
      const int cy = (contentTop + L.bottom) / 2;
      stick(g, L.w / 3 - 10, cy, pad, live);

      const int colX = 2 * L.w / 3 + 8;
      widgets::pill(g, colX, cy - 28, status, typeface::micro(), accent);
      chip(g, colX - 22, cy + 14, "A", frame_.buttons & 0x01);
      chip(g, colX + 22, cy + 14, "B", frame_.buttons & 0x02);
    } else {
      widgets::pill(g, L.cx, L.top + 8, status, typeface::micro(), accent);

      const int padTop = L.top + 36;
      const int pad = std::min(L.w - 44, L.bottom - padTop - 38);
      const int cy = padTop + pad / 2;
      stick(g, L.cx, cy, pad, live);

      const int rowY = cy + pad / 2 + 18;
      chip(g, L.cx - 26, rowY, "A", frame_.buttons & 0x01);
      chip(g, L.cx + 26, rowY, "B", frame_.buttons & 0x02);
    }

    widgets::hints(g, "BTN A", "BTN B");
  }

 private:
  const char* statusLabel(ShellContext& ctx, bool live) const {
    if (!gamepad_) return "UNAVAILABLE";
    if (!ctx.link.enabled()) return "BT OFF";
    if (live) return "LIVE";
    return ctx.link.connected() ? "CONNECTING" : "PAIR TO PLAY";
  }

  uint16_t statusColor(ShellContext& ctx, bool live) const {
    if (live) return theme::kHi;
    if (!gamepad_ || !ctx.link.enabled()) return theme::kDim;
    return theme::kWarn;
  }

  void stick(Gfx& g, int cx, int cy, int size, bool live) {
    auto& c = g.c();
    const int half = size / 2;

    c.drawRoundRect(cx - half, cy - half, size, size, 8, theme::kDim);
    c.drawFastHLine(cx - half + 4, cy, size - 8, theme::kDimmer);
    c.drawFastVLine(cx, cy - half + 4, size - 8, theme::kDimmer);

    const int reach = half - 9;
    const int dx = cx + frame_.x * reach / 127;
    const int dy = cy + frame_.y * reach / 127;
    c.fillCircle(dx, dy, 5, live ? theme::kHi : theme::kDim);
    c.drawCircle(dx, dy, 5, theme::kFg);
  }

  void chip(Gfx& g, int cx, int cy, const char* label, bool pressed) {
    auto& c = g.c();
    const int w = 36;
    const int h = 20;
    if (pressed) {
      c.fillRoundRect(cx - w / 2, cy - h / 2, w, h, 6, theme::kHi);
      g.str(label, cx, cy, theme::kBg, typeface::micro(), textdatum_t::middle_center);
    } else {
      c.drawRoundRect(cx - w / 2, cy - h / 2, w, h, 6, theme::kDim);
      g.str(label, cx, cy, theme::kFg, typeface::micro(), textdatum_t::middle_center);
    }
  }

  AnimClock sampler_;
  AnimClock redraw_;
  motion::TiltFilter filter_{kSmoothing};
  IGamepadLink* gamepad_ = nullptr;
  GamepadFrame frame_;
};

}  // namespace

TAMA_SCREEN_FACTORY(controller, ControllerScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_CONTROLLER
