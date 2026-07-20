#include "brand.gen.h"
#if TAMA_GAME_FLAPPY

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

class FlappyScreen : public ArcadeGameScreen {
 public:
  FlappyScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x9e3779b9u); }
  const char* id() const override { return "game.flappy"; }

 protected:
  const char* title() const override { return "FLAP"; }
  const char* readyHint() const override { return "A TO FLY"; }
  const char* runHint() const override { return "FLAP"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    const int gameH = h_ - kFloorH;
    pipe_.draw(g.c(), gameH);
    g.c().drawFastHLine(0, gameH, w_, theme::kDim);

    const Expr e = body_.v < 0 ? Expr::Happy : Expr::Neutral;
    player(g, ctx, kBirdX, static_cast<int>(body_.p), 22, e, 0, false);
  }

  Transition onAction(Intent intent, ShellContext&) override {
    if (intent != Intent::Select) return Transition::none();
    body_.v = kFlap;
    return Transition::redraw();
  }

  void onReset() override {
    body_.rest(h_ / 2.0f);
    pipe_.reset(rng_, w_, h_ - kFloorH);
    passed_ = false;
  }

  void step(ShellContext&) override {
    body_.integrate(kGravity, kStepSec);

    if (pipe_.advance(kPipeSpeed * kStepSec, rng_, w_, h_ - kFloorH)) passed_ = false;

    const int gameH = h_ - kFloorH;
    if (body_.p + kBirdR > gameH || body_.p - kBirdR < 0) return die();
    if (pipe_.blocks(kBirdX, body_.p, kBirdR)) return die();
    if (!passed_ && pipe_.cleared(kBirdX, kBirdR)) {
      passed_ = true;
      ++score_;
    }
  }

 private:
  static constexpr int kBirdX = 34;
  static constexpr int kBirdR = 11;
  static constexpr int kFloorH = 20;
  static constexpr float kGravity = 490.0f;
  static constexpr float kFlap = -115.0f;
  static constexpr float kPipeSpeed = 100.0f;

  Body1D body_;
  GapChannel pipe_{18, 62, 18};
  bool passed_ = false;
};

}  // namespace

TAMA_SCREEN_FACTORY(flappy, FlappyScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_FLAPPY
