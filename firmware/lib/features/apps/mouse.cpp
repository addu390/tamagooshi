#include "brand.gen.h"
#if TAMA_APP_MOUSE

#include <algorithm>
#include <cmath>

#include "apps.h"
#include "hidsession.h"
#include "motion.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr float kSmoothing = 0.35f;
constexpr float kFullTilt = 0.50f;
constexpr float kDeadzone = 0.05f;
constexpr int8_t kMaxStep = 18;
constexpr uint32_t kSampleMs = 20;
constexpr uint32_t kRedrawMs = 50;

int8_t toMove(float tilt) {
  if (std::fabs(tilt) < kDeadzone) return 0;
  const float scaled = std::clamp(tilt / kFullTilt, -1.0f, 1.0f);
  return static_cast<int8_t>(std::lround(scaled * kMaxStep));
}

class MouseScreen : public AppScreen {
 public:
  const char* id() const override { return "app.mouse"; }

  void onEnter(ShellContext& ctx) override {
    hid_.enter(ctx);
    filter_.reset();
    zeroed_ = false;
    zeroX_ = zeroY_ = 0.0f;
    tiltX_ = tiltY_ = 0.0f;
    buttons_ = 0;
  }

  void onExit() override { hid_.exit(); }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (!sampler_.due(nowMs, kSampleMs)) return Transition::none();

    if (filter_.sample(&ctx.sensor)) {
      float tx = filter_.x();
      float ty = filter_.y();
      if (zeroed_) {
        tx -= zeroX_;
        ty -= zeroY_;
      }
      tiltX_ = tx;
      tiltY_ = ty;
    }

    const int8_t dx = toMove(tiltX_);
    const int8_t dy = toMove(-tiltY_);
    buttons_ = buttonsFrom(ctx);

    if (hid_.live()) hid_.pointer(dx, dy, buttons_);

    return redraw_.due(nowMs, kRedrawMs) ? Transition::redraw() : Transition::none();
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    if (intent != Intent::Select) return Transition::none();
    if (zeroed_) {
      zeroed_ = false;
      zeroX_ = zeroY_ = 0.0f;
    } else {
      zeroed_ = true;
      zeroX_ = filter_.x();
      zeroY_ = filter_.y();
    }
    return Transition::redraw();
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "MOUSE");
    hid_.statusPill(g, ctx, L.cx, L.top + 20);

    float sx = tiltX_;
    float sy = tiltY_;
    if (g.rotation() == 1) {
      sx = tiltY_;
      sy = -tiltX_;
    }

    const int padTop = L.top + 44;
    const int pad = std::min(L.w - 44, L.bottom - padTop - 40);
    const int cy = padTop + pad / 2;
    crosshair(g, L.cx, cy, pad, sx, sy, hid_.live());

    const int rowY = cy + pad / 2 + 16;
    chip(g, L.cx - 26, rowY, "A", buttons_ & kMouseBtnLeft);
    chip(g, L.cx + 26, rowY, "B", buttons_ & kMouseBtnRight);

    widgets::hints(g, zeroed_ ? "RESET" : "ZERO", "BACK");
  }

 private:
  static uint8_t buttonsFrom(ShellContext& ctx) {
    uint8_t buttons = 0;
    if (ctx.buttons.held(0)) buttons |= kMouseBtnLeft;
    if (ctx.buttons.held(1)) buttons |= kMouseBtnRight;
    return buttons;
  }

  void crosshair(Gfx& g, int cx, int cy, int size, float tx, float ty, bool live) {
    auto& c = g.c();
    const int half = size / 2;

    c.drawRoundRect(cx - half, cy - half, size, size, 8, theme::kDim);
    c.drawFastHLine(cx - half + 4, cy, size - 8, theme::kDimmer);
    c.drawFastVLine(cx, cy - half + 4, size - 8, theme::kDimmer);

    const int reach = half - 9;
    const float mag = std::clamp(std::sqrt(tx * tx + ty * ty) / kFullTilt, 0.0f, 1.0f);
    const int px = cx + static_cast<int>(tx / kFullTilt * reach);
    const int py = cy + static_cast<int>(ty / kFullTilt * reach);
    const uint16_t col = live ? theme::kHi : theme::kDim;
    if (mag > kDeadzone) {
      c.fillCircle(px, py, 5, col);
      c.drawCircle(px, py, 5, theme::kFg);
    } else {
      c.drawCircle(cx, cy, 3, col);
    }
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

  MouseSession hid_;
  AnimClock sampler_;
  AnimClock redraw_;
  motion::TiltFilter filter_{kSmoothing};
  bool zeroed_ = false;
  float zeroX_ = 0.0f;
  float zeroY_ = 0.0f;
  float tiltX_ = 0.0f;
  float tiltY_ = 0.0f;
  uint8_t buttons_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(mouse, MouseScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_MOUSE
