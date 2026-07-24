#include "brand.gen.h"
#if TAMA_GAME_SCREAM

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

class ScreamScreen : public ArcadeGameScreen {
 public:
  ScreamScreen() : ArcadeGameScreen(OrientationPref::Landscape) { rng_.seed(0x51ed270bu); }
  const char* id() const override { return "game.scream"; }

  void onEnter(ShellContext& ctx) override {
    ArcadeGameScreen::onEnter(ctx);
    mic_ = &ctx.mic;
    mic_->begin();
  }

  void onExit() override {
    if (mic_) {
      mic_->end();
      mic_ = nullptr;
    }
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (st_ != St::Run && idle_.due(nowMs, kIdleSampleMs)) sample();
    return ArcadeGameScreen::tick(ctx, nowMs);
  }

 protected:
  const char* title() const override { return "SCREAM"; }
  const char* readyHint() const override { return "YELL TO FLY"; }
  const char* runHint() const override { return "YELL!"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    wall_.draw(g.c(), h_);
    drawMeter(g);

    const Expr e = loud_ ? Expr::Happy : Expr::Neutral;
    player(g, ctx, kHeroX, static_cast<int>(y_), 28, e, 0, false);
  }

  void onReset() override {
    y_ = h_ / 2.0f;
    wall_.reset(rng_, w_, h_);
    loud_ = false;
  }

  void step(ShellContext&) override {
    sample();

    y_ += loud_ ? -kRiseRate * kStepSec : kFallRate * kStepSec;
    if (y_ < kHeroR) y_ = kHeroR;

    if (wall_.advance(kWallRate * kStepSec, rng_, w_, h_)) ++score_;

    if (y_ > h_) return die();
    if (wall_.blocks(kHeroX, y_, kHeroR)) return die();
  }

 private:
  void sample() {
    level_ = mic_ ? mic_->level() : 0;
    if (baseline_ < 0) {
      baseline_ = level_;
    } else {
      baseline_ += (level_ - baseline_) / (level_ < baseline_ ? 4 : 128);
    }
    threshold_ = baseline_ * 2 + kMinDelta;
    loud_ = level_ > threshold_;
  }

  void drawMeter(Gfx& g) {
    auto& c = g.c();
    const int y = h_ - kMeterH;
    int len = w_ * level_ / (threshold_ * 2);
    if (len > w_) len = w_;
    c.fillRect(0, y, w_, kMeterH, theme::kBg);
    if (len > 0) c.fillRect(0, y, len, kMeterH, loud_ ? theme::kHi : theme::kDim);
    c.drawFastVLine(w_ / 2, y - 2, kMeterH + 2, theme::kFg);
  }

  static constexpr int kHeroX = 40;
  static constexpr int kHeroR = 8;
  static constexpr int kMinDelta = 200;
  static constexpr int kMeterH = 3;
  static constexpr uint32_t kIdleSampleMs = 150;
  static constexpr float kRiseRate = 180.0f;
  static constexpr float kFallRate = 48.0f;
  static constexpr float kWallRate = 48.0f;

  IMicSource* mic_ = nullptr;
  GapChannel wall_{20, 66, 10};
  AnimClock idle_;
  float y_ = 67;
  int level_ = 0;
  int baseline_ = -1;
  int threshold_ = kMinDelta;
  bool loud_ = false;
};

}  // namespace

TAMA_SCREEN_FACTORY(scream, ScreamScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_SCREAM
