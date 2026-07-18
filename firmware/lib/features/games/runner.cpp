#include "brand.gen.h"
#if TAMA_GAME_RUNNER

#include <string>

#include "arcade.h"
#include "games.h"
#include "mascot.h"
#include "widgets.h"

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

  void render(Gfx& g, ShellContext& ctx) override {
    w_ = g.w();
    h_ = g.h();
    const int groundY = h_ - 26;
    auto& c = g.c();

    c.drawFastHLine(0, groundY, w_, theme::kDim);
    for (int x = (scroll_ % 16); x < w_; x += 16) c.drawPixel(x, groundY + 4, theme::kDimmer);

    for (const auto& o : obs_.items) {
      drawCactus(c, static_cast<int>(o.x), groundY, o.w, o.h, theme::kFg);
    }

    const int cy = groundY - 16;
    const Expr e = st_ == St::Dead ? Expr::Alert : (py_ > 0.5f ? Expr::Happy : Expr::Neutral);
    if (ctx.character)
      ctx.character->draw(g, kPlayerX, cy, 28, MascotState{e, 0, true, static_cast<int>(py_)}, now_);

    g.str(std::to_string(score_).c_str(), w_ - 6, 4, theme::kHi, typeface::title(),
          textdatum_t::top_right);

    if (st_ == St::Ready) {
      banner(g, "DASH", "A TO JUMP");
    } else if (st_ == St::Dead) {
      banner(g, "GAME OVER", ("BEST " + std::to_string(best_)).c_str());
    }
    widgets::hints(g, st_ == St::Dead ? "RETRY" : "JUMP", nullptr);
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent != Intent::Select) return Transition::none();
    if (st_ != St::Run) {
      begin();
      return Transition::redraw();
    }
    if (py_ <= 0.5f) vy_ = kJumpV;
    return Transition::redraw();
  }

 protected:
  void onReset() override {
    py_ = 0;
    vy_ = 0;
    speed_ = kSpeed0;
    scroll_ = 0;
    gap_ = 130;
    obs_.clear();
  }

  void step(ShellContext&) override {
    vy_ -= kGravity * kStepSec;
    py_ += vy_ * kStepSec;
    if (py_ <= 0) {
      py_ = 0;
      vy_ = 0;
    }

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
      if (kPlayerX + kPlayerHalf > ox0 && kPlayerX - kPlayerHalf < ox1 && py_ < o.h) {
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
  static constexpr float kStepSec = 0.016f;
  static constexpr float kJumpV = 340.0f;
  static constexpr float kGravity = 1100.0f;
  static constexpr float kSpeed0 = 150.0f;
  static constexpr float kSpeedStep = 25.0f;
  static constexpr float kSpeedMax = 380.0f;

  float py_ = 0;
  float vy_ = 0;
  float speed_ = kSpeed0;
  int scroll_ = 0;
  int gap_ = 130;
  HorizSpawner<Obstacle> obs_;
};

}  // namespace

AppScreen& runner() {
  static RunnerScreen instance;
  return instance;
}

}  // namespace tama::games

#endif  // TAMA_GAME_RUNNER
