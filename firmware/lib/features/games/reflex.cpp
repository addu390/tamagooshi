#include "brand.gen.h"
#if TAMA_GAME_REFLEX

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

class ReflexScreen : public ArcadeGameScreen {
 public:
  ReflexScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x9e37f1e5u); }
  const char* id() const override { return "game.reflex"; }

 protected:
  const char* title() const override { return "REFLEX"; }
  const char* readyHint() const override { return "TAP A ON GREEN"; }
  const char* runHint() const override { return "TAP!"; }
  const char* deadTitle() const override { return armed_ ? "TOO SLOW" : "TOO EAGER"; }
  int bannerCenterY() const override { return circleCy() - kCircleR - 18; }
  const lgfx::IFont* bannerHeadFont() const override { return typeface::body(); }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();
    const int cx = w_ / 2;
    const int cy = circleCy();

    if (st_ == St::Run && armed_) {
      c.fillCircle(cx, cy, kCircleR, theme::kHi);
      g.str("TAP!", cx, cy, theme::kBg, typeface::title(), textdatum_t::middle_center);
      const int left = static_cast<int>(window_) - static_cast<int>(phaseMs_);
      const int barW = (w_ - 40) * (left > 0 ? left : 0) / static_cast<int>(window_);
      c.fillRect(20, cy + kCircleR + 12, barW, 4, theme::kWarn);
    } else {
      c.drawCircle(cx, cy, kCircleR, theme::kDim);
      if (st_ == St::Run) {
        g.str("WAIT", cx, cy, theme::kDim, typeface::body(), textdatum_t::middle_center);
      }
    }

    player(g, ctx, cx, h_ - 34, 26, armed_ ? Expr::Alert : Expr::Neutral);
  }

  Transition onAction(Intent intent, ShellContext& ctx) override {
    if (intent != Intent::Select) return Transition::none();
    if (!armed_) {
      die();
      return Transition::redraw();
    }
    ++score_;
    cue(ctx, ExpressionKind::Chirp);
    rearm();
    return Transition::redraw();
  }

  void onReset() override { rearm(); }

  void step(ShellContext&) override {
    phaseMs_ += kStepMs;
    if (!armed_) {
      if (phaseMs_ >= waitMs_) {
        armed_ = true;
        phaseMs_ = 0;
        window_ = kWindowRamp.at(static_cast<float>(score_));
      }
      return;
    }
    if (phaseMs_ >= static_cast<uint32_t>(window_)) die();
  }

 private:
  void rearm() {
    armed_ = false;
    phaseMs_ = 0;
    waitMs_ = 700 + rng_.next() % 1800u;
  }

  int circleCy() const { return h_ / 2 - 8; }

  static constexpr int kCircleR = 34;
  static constexpr DifficultyRamp kWindowRamp{620.0f, -14.0f, 280.0f, 0.0f};

  bool armed_ = false;
  uint32_t phaseMs_ = 0;
  uint32_t waitMs_ = 1200;
  float window_ = 620.0f;
};

}  // namespace

TAMA_SCREEN_FACTORY(reflex, ReflexScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_REFLEX
