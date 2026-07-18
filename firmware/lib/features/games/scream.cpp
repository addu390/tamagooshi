#include "brand.gen.h"
#if TAMA_GAME_SCREAM

#include <string>

#include "arcade.h"
#include "games.h"
#include "input.h"
#include "mascot.h"
#include "widgets.h"

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

  void render(Gfx& g, ShellContext& ctx) override {
    w_ = g.w();
    h_ = g.h();
    auto& c = g.c();

    const int top = gapY_;
    const int bot = gapY_ + kGapH;
    const int wx = static_cast<int>(wallX_);
    c.fillRect(wx, 0, kWallW, top, theme::kDimmer);
    c.drawRect(wx, 0, kWallW, top, theme::kFg);
    c.fillRect(wx, bot, kWallW, h_ - bot, theme::kDimmer);
    c.drawRect(wx, bot, kWallW, h_ - bot, theme::kFg);

    drawMeter(g);

    const Expr e = st_ == St::Dead ? Expr::Alert : (loud_ ? Expr::Happy : Expr::Neutral);
    if (ctx.character) {
      ctx.character->draw(g, kHeroX, static_cast<int>(y_), 28, MascotState{e, 0, true, 0, false},
                          now_);
    }

    g.str(std::to_string(score_).c_str(), w_ / 2, 6, theme::kHi, typeface::title(),
          textdatum_t::top_center);

    if (st_ == St::Ready) {
      banner(g, "SCREAM", "YELL TO FLY");
    } else if (st_ == St::Dead) {
      banner(g, "GAME OVER", ("BEST " + std::to_string(best_)).c_str());
    }
    widgets::hints(g, st_ == St::Dead ? "RETRY" : "YELL!", nullptr);
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Select && st_ != St::Run) {
      begin();
      return Transition::redraw();
    }
    return Transition::none();
  }

 protected:
  void onReset() override {
    y_ = h_ / 2.0f;
    wallX_ = static_cast<float>(w_);
    gapY_ = spawnGap();
    level_ = 0;
    loud_ = false;
  }

  void step(ShellContext&) override {
    level_ = mic_ ? mic_->level() : 0;
    loud_ = level_ > kThreshold;

    y_ += loud_ ? -kRiseRate * kStepSec : kFallRate * kStepSec;

    wallX_ -= kWallRate * kStepSec;
    if (wallX_ < -kWallW) {
      wallX_ = static_cast<float>(w_);
      gapY_ = spawnGap();
      ++score_;
    }

    if (y_ < 0 || y_ > h_) return die();
    const int wx = static_cast<int>(wallX_);
    if (kHeroX + kHeroR > wx && kHeroX - kHeroR < wx + kWallW) {
      if (y_ - kHeroR < gapY_ || y_ + kHeroR > gapY_ + kGapH) return die();
    }
  }

 private:
  int spawnGap() {
    const int span = h_ - kGapH - 2 * kMargin;
    return kMargin + static_cast<int>(rng_.next() % static_cast<uint32_t>(span > 1 ? span : 1));
  }

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
  static constexpr int kWallW = 20;
  static constexpr int kGapH = 50;
  static constexpr int kMargin = 10;
  static constexpr int kThreshold = 250;
  static constexpr int kMeterMax = 1000;
  static constexpr int kMeterH = 3;
  static constexpr float kStepSec = 0.016f;
  static constexpr float kRiseRate = 200.0f;
  static constexpr float kFallRate = 67.0f;
  static constexpr float kWallRate = 67.0f;

  IMicSource* mic_ = nullptr;
  float y_ = 67;
  float wallX_ = 240;
  int gapY_ = 40;
  int level_ = 0;
  bool loud_ = false;
};

}  // namespace

AppScreen& scream() {
  static ScreamScreen instance;
  return instance;
}

}  // namespace tama::games

#endif  // TAMA_GAME_SCREAM
