#include "brand.gen.h"
#if TAMA_GAME_FLAPPY

#include <string>

#include "arcade.h"
#include "games.h"
#include "mascot.h"
#include "widgets.h"

namespace tama::games {

namespace {

class FlappyScreen : public ArcadeGameScreen {
 public:
  FlappyScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x9e3779b9u); }
  const char* id() const override { return "game.flappy"; }

  void render(Gfx& g, ShellContext& ctx) override {
    w_ = g.w();
    h_ = g.h();
    const int gameH = h_ - kFloorH;
    auto& c = g.c();

    const int px = static_cast<int>(pipeX_);
    c.fillRect(px, 0, kPipeW, gapY_, theme::kDimmer);
    c.drawRect(px, 0, kPipeW, gapY_, theme::kFg);
    c.fillRect(px, gapY_ + kGapH, kPipeW, gameH - (gapY_ + kGapH), theme::kDimmer);
    c.drawRect(px, gapY_ + kGapH, kPipeW, gameH - (gapY_ + kGapH), theme::kFg);

    c.drawFastHLine(0, gameH, w_, theme::kDim);

    const Expr e = st_ == St::Dead ? Expr::Alert : (vy_ < 0 ? Expr::Happy : Expr::Neutral);
    if (ctx.character) {
      ctx.character->draw(g, kBirdX, static_cast<int>(y_), 22, MascotState{e, 0, true, 0, false},
                          now_);
    }

    g.str(std::to_string(score_).c_str(), w_ / 2, 6, theme::kHi, typeface::title(),
          textdatum_t::top_center);

    if (st_ == St::Ready) {
      banner(g, "FLAP", "A TO FLY");
    } else if (st_ == St::Dead) {
      banner(g, "GAME OVER", ("BEST " + std::to_string(best_)).c_str());
    }
    widgets::hints(g, st_ == St::Dead ? "RETRY" : "FLAP", nullptr);
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent != Intent::Select) return Transition::none();
    if (st_ != St::Run) {
      begin();
      return Transition::redraw();
    }
    vy_ = kFlap;
    return Transition::redraw();
  }

 protected:
  void onReset() override {
    y_ = h_ / 2.0f;
    vy_ = 0;
    pipeX_ = static_cast<float>(w_);
    gapY_ = spawnGap();
    passed_ = false;
  }

  void step(ShellContext&) override {
    vy_ += kGravity * kStepSec;
    y_ += vy_ * kStepSec;

    pipeX_ -= kPipeSpeed * kStepSec;
    if (pipeX_ < -kPipeW) {
      pipeX_ = static_cast<float>(w_);
      gapY_ = spawnGap();
      passed_ = false;
    }

    const int gameH = h_ - kFloorH;
    if (y_ + kBirdR > gameH || y_ - kBirdR < 0) return die();
    const int px = static_cast<int>(pipeX_);
    if (kBirdX + kBirdR > px && kBirdX - kBirdR < px + kPipeW) {
      if (y_ - kBirdR < gapY_ || y_ + kBirdR > gapY_ + kGapH) return die();
    }
    if (!passed_ && px + kPipeW < kBirdX - kBirdR) {
      passed_ = true;
      ++score_;
    }
  }

 private:
  int spawnGap() {
    const int gameH = h_ - kFloorH;
    const int span = gameH - kGapH - 2 * kMargin;
    return kMargin + static_cast<int>(rng_.next() % static_cast<uint32_t>(span > 1 ? span : 1));
  }

  static constexpr int kBirdX = 34;
  static constexpr int kBirdR = 11;
  static constexpr int kPipeW = 18;
  static constexpr int kGapH = 62;
  static constexpr int kFloorH = 20;
  static constexpr int kMargin = 18;
  static constexpr float kStepSec = 0.016f;
  static constexpr float kGravity = 490.0f;
  static constexpr float kFlap = -115.0f;
  static constexpr float kPipeSpeed = 100.0f;

  float y_ = 120;
  float vy_ = 0;
  float pipeX_ = 135;
  int gapY_ = 90;
  bool passed_ = false;
};

}  // namespace

AppScreen& flappy() {
  static FlappyScreen instance;
  return instance;
}

}  // namespace tama::games

#endif  // TAMA_GAME_FLAPPY
