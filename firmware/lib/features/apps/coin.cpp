#include "brand.gen.h"
#if TAMA_APP_COIN

#include <cmath>
#include <cstdio>

#include "apps.h"
#include "rng.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr uint32_t kFlipMs = 800;
constexpr float kSpins = 5.0f;

class CoinScreen : public AppScreen {
 public:
  const char* id() const override { return "app.coin"; }

  void onEnter(ShellContext&) override {
    rng_.seed(0xc0111f11u ^ now());
    heads_ = 0;
    tails_ = 0;
    flipping_ = false;
    isHeads_ = true;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "COIN");

    const int r = L.landscape ? 34 : 42;
    const int cy = L.cy - 6;
    float squash = 1.0f;
    bool showHeads = isHeads_;
    if (flipping_) {
      const float t = static_cast<float>(now() - flipStart_) / kFlipMs;
      const float angle = t * kSpins * 3.14159265f;
      squash = std::fabs(std::cos(angle));
      showHeads = std::cos(angle) >= 0 ? faceAtStart_ : !faceAtStart_;
    }

    auto& c = g.c();
    const int ry = static_cast<int>(r * squash);
    c.fillEllipse(L.cx, cy, r, ry > 2 ? ry : 2, theme::kHi);
    c.drawEllipse(L.cx, cy, r, ry > 2 ? ry : 2, theme::kFg);
    if (squash > 0.55f) {
      g.str(showHeads ? "H" : "T", L.cx, cy, theme::kBg, typeface::title(),
            textdatum_t::middle_center);
    }

    if (!flipping_) {
      widgets::pill(g, L.cx, cy + r + 10, isHeads_ ? "HEADS" : "TAILS", typeface::micro(),
                    theme::kHi);
    }

    char tally[20];
    std::snprintf(tally, sizeof(tally), "H %d   T %d", heads_, tails_);
    g.str(tally, L.cx, L.bottom - 8, theme::kDim, typeface::micro(), textdatum_t::middle_center);

    widgets::hints(g, "FLIP", "CLEAR");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select && !flipping_) {
      flipping_ = true;
      flipStart_ = now();
      faceAtStart_ = isHeads_;
      cue(ctx, ExpressionKind::Haptic);
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      heads_ = 0;
      tails_ = 0;
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    AppScreen::tick(ctx, nowMs);
    if (!flipping_) return Transition::none();

    if (nowMs - flipStart_ >= kFlipMs) {
      flipping_ = false;
      isHeads_ = (rng_.next() & 1u) == 0;
      if (isHeads_) {
        ++heads_;
      } else {
        ++tails_;
      }
      cue(ctx, ExpressionKind::Chirp);
    }
    return Transition::redraw();
  }

 private:
  LcgRng rng_{0xc01f11b5u};
  bool flipping_ = false;
  bool isHeads_ = true;
  bool faceAtStart_ = true;
  uint32_t flipStart_ = 0;
  int heads_ = 0;
  int tails_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(coin, CoinScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_COIN
