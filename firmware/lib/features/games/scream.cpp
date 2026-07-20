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
    level_ = 0;
    loud_ = false;
  }

  void step(ShellContext&) override {
    level_ = mic_ ? mic_->level() : 0;
    loud_ = level_ > kThreshold;

    y_ += loud_ ? -kRiseRate * kStepSec : kFallRate * kStepSec;

    if (wall_.advance(kWallRate * kStepSec, rng_, w_, h_)) ++score_;

    if (y_ < 0 || y_ > h_) return die();
    if (wall_.blocks(kHeroX, y_, kHeroR)) return die();
  }

 private:
  void drawMeter(Gfx& g) {
    auto& c = g.c();
    const int y = h_ - kMeterH;
    int len = w_ * level_ / kMeterMax;
    if (len > w_) len = w_;
    c.fillRect(0, y, w_, kMeterH, theme::kBg);
    if (len > 0) c.fillRect(0, y, len, kMeterH, loud_ ? theme::kHi : theme::kDim);
  }

  static constexpr int kHeroX = 40;
  static constexpr int kHeroR = 8;
  static constexpr int kThreshold = 250;
  static constexpr int kMeterMax = 1000;
  static constexpr int kMeterH = 3;
  static constexpr float kRiseRate = 200.0f;
  static constexpr float kFallRate = 67.0f;
  static constexpr float kWallRate = 67.0f;

  IMicSource* mic_ = nullptr;
  GapChannel wall_{20, 50, 10};
  float y_ = 67;
  int level_ = 0;
  bool loud_ = false;
};

}  // namespace

TAMA_SCREEN_FACTORY(scream, ScreamScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_SCREAM
