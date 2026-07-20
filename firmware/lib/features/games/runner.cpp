#include "brand.gen.h"
#if TAMA_GAME_RUNNER

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

struct Obstacle {
  float x;
  int w;
  int h;
  bool scored;
};

void drawCactus(M5Canvas& c, int x, int baseY, int w, int h, uint16_t col) {
  const int stemW = 5;
  const int cx = x + (w - stemW) / 2;
  c.fillRect(cx, baseY - h, stemW, h, col);

  const int lay = baseY - (h * 6 / 10);
  c.fillRect(x, lay, cx - x, 3, col);
  c.fillRect(x, lay - 5, 3, 8, col);

  if (h >= 22) {
    const int ray = baseY - (h * 45 / 100);
    const int rx = cx + stemW;
    c.fillRect(rx, ray, (x + w) - rx, 3, col);
    c.fillRect(x + w - 3, ray - 5, 3, 8, col);
  }
}

class RunnerScreen : public ArcadeGameScreen {
 public:
  RunnerScreen() : ArcadeGameScreen(OrientationPref::Landscape) { rng_.seed(0x2545f491u); }
  const char* id() const override { return "game.runner"; }

 protected:
  const char* title() const override { return "DASH"; }
  const char* readyHint() const override { return "A TO JUMP"; }
  const char* runHint() const override { return "JUMP"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    const int groundY = h_ - 26;
    auto& c = g.c();

    c.drawFastHLine(0, groundY, w_, theme::kDim);
    for (int x = (scroll_ % 16); x < w_; x += 16) c.drawPixel(x, groundY + 4, theme::kDimmer);

    for (const auto& o : obs_.items) {
      drawCactus(c, static_cast<int>(o.x), groundY, o.w, o.h, theme::kFg);
    }

    const Expr e = body_.p > 0.5f ? Expr::Happy : Expr::Neutral;
    player(g, ctx, kPlayerX, groundY - 16, 28, e, static_cast<int>(body_.p));
  }

  Transition onAction(Intent intent, ShellContext&) override {
    if (intent != Intent::Select) return Transition::none();
    if (body_.p <= 0.5f) body_.v = kJumpV;
    return Transition::redraw();
  }

  void onReset() override {
    body_ = {};
    speed_ = kSpeed0;
    scroll_ = 0;
    gap_ = 130;
    obs_.clear();
  }

  void step(ShellContext&) override {
    body_.integrate(-kGravity, kStepSec);
    if (body_.p <= 0) body_.rest(0);

    scroll_ = (scroll_ + static_cast<int>(speed_ * kStepSec)) % 16;

    updateSpawns();
    checkCollisions();
  }

 private:
  void updateSpawns() {
    obs_.advance(speed_ * kStepSec);
    auto& items = obs_.items;
    if (!items.empty() && items.front().x + items.front().w < 0) items.erase(items.begin());

    if (obs_.due(w_ - gap_, 0)) {
      Obstacle o;
      o.x = static_cast<float>(w_ + 4);
      o.w = 8 + static_cast<int>(rng_.next() % 7u);
      o.h = (rng_.next() & 1u) ? kTallH : kShortH;
      o.scored = false;
      items.push_back(o);
      gap_ = 100 + static_cast<int>(rng_.next() % 40u) + static_cast<int>(speed_ * 0.20f);
    }
  }

  void checkCollisions() {
    for (auto& o : obs_.items) {
      const int ox0 = static_cast<int>(o.x);
      const int ox1 = ox0 + o.w;
      if (kPlayerX + kPlayerHalf > ox0 && kPlayerX - kPlayerHalf < ox1 && body_.p < o.h) {
        return die();
      }
      if (!o.scored && ox1 < kPlayerX - kPlayerHalf) {
        o.scored = true;
        ++score_;
        if (score_ % 5 == 0 && speed_ < kSpeedMax) speed_ += kSpeedStep;
      }
    }
  }

  static constexpr int kPlayerX = 24;
  static constexpr int kPlayerHalf = 10;
  static constexpr int kShortH = 16;
  static constexpr int kTallH = 26;
  static constexpr float kJumpV = 340.0f;
  static constexpr float kGravity = 1100.0f;
  static constexpr float kSpeed0 = 150.0f;
  static constexpr float kSpeedStep = 25.0f;
  static constexpr float kSpeedMax = 380.0f;

  Body1D body_;
  float speed_ = kSpeed0;
  int scroll_ = 0;
  int gap_ = 130;
  HorizSpawner<Obstacle> obs_;
};

}  // namespace

TAMA_SCREEN_FACTORY(runner, RunnerScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_RUNNER
