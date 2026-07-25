#include "brand.gen.h"
#if TAMA_APP_DICE

#include <cmath>
#include <cstdio>

#include "apps.h"
#include "rng.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr uint32_t kRollMs = 750;
constexpr uint32_t kTumbleMs = 90;
constexpr uint32_t kShakeCooldownMs = 900;
constexpr float kShakeG = 1.7f;

class DiceScreen : public AppScreen {
 public:
  const char* id() const override { return "app.dice"; }

  void onEnter(ShellContext&) override {
    rng_.seed(0xd1ce0000u ^ now());
    count_ = 1;
    rolling_ = false;
    settle();
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "DICE");

    const int size = L.landscape ? 52 : 62;
    const int gap = 12;
    const int totalW = count_ * size + (count_ - 1) * gap;
    int x = L.cx - totalW / 2 + size / 2;
    const int cy = L.cy - 4;
    for (int i = 0; i < count_; ++i) {
      die(g, x, cy, size, values_[i]);
      x += size + gap;
    }

    if (!rolling_ && count_ == 2) {
      char sum[8];
      std::snprintf(sum, sizeof(sum), "= %d", values_[0] + values_[1]);
      g.str(sum, L.cx, cy + size / 2 + 16, theme::kDim, typeface::body(),
            textdatum_t::middle_center);
    }
    if (ctx.caps.imu) {
      g.str("OR SHAKE", L.cx, L.top + 24, theme::kDimmer, typeface::micro(),
            textdatum_t::middle_center);
    }

    widgets::hints(g, "ROLL", count_ == 1 ? "2 DICE" : "1 DIE");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) {
      roll(ctx);
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      count_ = count_ == 1 ? 2 : 1;
      settle();
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    AppScreen::tick(ctx, nowMs);

    if (rolling_) {
      if (nowMs - rollStart_ >= kRollMs) {
        rolling_ = false;
        settle();
        cue(ctx, ExpressionKind::Chirp);
      } else if (tumble_.due(nowMs, kTumbleMs)) {
        settle();
      }
      return Transition::redraw();
    }

    if (ctx.caps.imu && shaken(ctx, nowMs)) roll(ctx);
    return Transition::none();
  }

 private:
  void roll(ShellContext& ctx) {
    rolling_ = true;
    rollStart_ = now();
    cue(ctx, ExpressionKind::Haptic);
  }

  void settle() {
    for (int i = 0; i < 2; ++i) values_[i] = 1 + static_cast<int>(rng_.next() % 6u);
  }

  bool shaken(ShellContext& ctx, uint32_t nowMs) {
    float ax, ay, az;
    if (!ctx.sensor.accel(ax, ay, az)) return false;
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (mag < kShakeG) return false;
    if (nowMs - lastShake_ < kShakeCooldownMs) return false;
    lastShake_ = nowMs;
    return true;
  }

  void die(Gfx& g, int cx, int cy, int size, int value) {
    auto& c = g.c();
    const int half = size / 2;
    c.fillRoundRect(cx - half, cy - half, size, size, 8, theme::kFg);
    c.drawRoundRect(cx - half, cy - half, size, size, 8, theme::kDim);

    const int o = size / 4;
    const int r = size / 10;
    const bool mid = value % 2 == 1;
    const bool corners = value >= 2;
    const bool sides = value >= 4;
    const bool edges = value == 6;
    if (mid) pip(c, cx, cy, r);
    if (corners) {
      pip(c, cx - o, cy - o, r);
      pip(c, cx + o, cy + o, r);
    }
    if (sides) {
      pip(c, cx + o, cy - o, r);
      pip(c, cx - o, cy + o, r);
    }
    if (edges) {
      pip(c, cx - o, cy, r);
      pip(c, cx + o, cy, r);
    }
  }

  static void pip(M5Canvas& c, int x, int y, int r) { c.fillCircle(x, y, r, theme::kBg); }

  LcgRng rng_{0xd1ce5eedu};
  AnimClock tumble_;
  int values_[2] = {1, 1};
  int count_ = 1;
  bool rolling_ = false;
  uint32_t rollStart_ = 0;
  uint32_t lastShake_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(dice, DiceScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_DICE
