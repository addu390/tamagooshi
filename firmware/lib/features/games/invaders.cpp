#include "brand.gen.h"
#if TAMA_GAME_INVADERS

#include <algorithm>
#include <cmath>

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

class InvadersScreen : public ArcadeGameScreen {
 public:
  InvadersScreen() : ArcadeGameScreen(OrientationPref::Landscape) { rng_.seed(0x1a7ad375u); }
  const char* id() const override { return "game.invaders"; }

 protected:
  const char* title() const override { return "INVADERS"; }
  const char* readyHint() const override { return "A/B MOVE · A FIRE"; }
  const char* runHint() const override { return "FIRE"; }
  const char* hintA() const override {
    if (st_ == St::Dead) return "RETRY";
    if (st_ == St::Ready) return "START";
    return "FIRE";
  }
  const char* hintB() const override { return st_ == St::Run ? "MOVE" : nullptr; }
  const char* deadTitle() const override { return "OVERRUN"; }
  int bannerCenterY() const override { return 36; }
  const lgfx::IFont* bannerHeadFont() const override { return typeface::body(); }
  bool showScore() const override { return st_ != St::Ready; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();
    const bool chrome = st_ == St::Ready || st_ == St::Dead;
    const int yOff = chrome ? 40 : 0;

    for (int i = 0; i < kAliens; ++i) {
      if (!alive_[i]) continue;
      const int x = alienX(i);
      const int y = alienY(i) + yOff;
      c.fillRect(x - kAlienW / 2, y - kAlienH / 2, kAlienW, kAlienH, theme::kFg);
      c.drawRect(x - kAlienW / 2, y - kAlienH / 2, kAlienW, kAlienH, theme::kDim);
    }

    if (!chrome) {
      if (shot_.active) {
        c.fillRect(static_cast<int>(shot_.x) - 1, static_cast<int>(shot_.y) - 3, 2, 6, theme::kHi);
      }
      if (bomb_.active) {
        c.fillRect(static_cast<int>(bomb_.x) - 1, static_cast<int>(bomb_.y) - 3, 2, 6,
                    theme::kWarn);
      }
    }

    const int px = static_cast<int>(playerX_);
    const int py = playerY() - (chrome ? 6 : 0);
    c.fillTriangle(px, py - 5, px - 7, py + 4, px + 7, py + 4, theme::kHi);
    if (!chrome) player(g, ctx, px, py + 14, 18, Expr::Neutral, 0, false);
  }

  Transition onAction(Intent intent, ShellContext& ctx) override {
    if (intent != Intent::Select) return Transition::none();
    if (shot_.active) return Transition::none();
    shot_.active = true;
    shot_.x = playerX_;
    shot_.y = static_cast<float>(playerY() - 8);
    cue(ctx, ExpressionKind::Chirp);
    return Transition::redraw();
  }

  void onReset() override {
    playerX_ = w_ / 2.0f;
    originX_ = 18.0f;
    originY_ = 22.0f;
    dir_ = 1;
    moveAcc_ = 0;
    shot_.active = false;
    bomb_.active = false;
    wave_ = 0;
    resetAliens();
  }

  void step(ShellContext& ctx) override {
    const float dt = kStepSec;
    if (ctx.buttons.held(0)) playerX_ -= kMoveSpeed * dt;
    if (ctx.buttons.held(1)) playerX_ += kMoveSpeed * dt;
    const float half = 8.0f;
    if (playerX_ < half) playerX_ = half;
    if (playerX_ > w_ - half) playerX_ = w_ - half;

    moveAcc_ += kStepMs;
    const uint32_t pace = static_cast<uint32_t>(kPaceRamp.at(elapsedSec()) - wave_ * 18.0f);
    const uint32_t interval = pace < 120 ? 120 : pace;
    if (moveAcc_ >= interval) {
      moveAcc_ = 0;
      stepAliens();
    }

    if (shot_.active) {
      shot_.y -= kShotSpeed * dt;
      if (shot_.y < 8) {
        shot_.active = false;
      } else {
        hitAliens(ctx);
      }
    }

    if (bomb_.active) {
      bomb_.y += kBombSpeed * dt;
      if (bomb_.y > h_ - 4) {
        bomb_.active = false;
      } else if (std::fabs(bomb_.x - playerX_) < 8.0f &&
                 bomb_.y > playerY() - 6 && bomb_.y < playerY() + 8) {
        die();
        return;
      }
    } else if (left_ > 0 && rng_.unit() < kBombChance) {
      dropBomb();
    }

    if (lowestAlienY() >= playerY() - 12) die();
  }

 private:
  struct Bolt {
    float x = 0;
    float y = 0;
    bool active = false;
  };

  int playerY() const { return h_ - 28; }

  int alienX(int i) const {
    const int col = i % kCols;
    return static_cast<int>(originX_) + col * kStrideX;
  }

  int alienY(int i) const {
    const int row = i / kCols;
    return static_cast<int>(originY_) + row * kStrideY;
  }

  int lowestAlienY() const {
    int lo = 0;
    for (int i = 0; i < kAliens; ++i) {
      if (!alive_[i]) continue;
      lo = std::max(lo, alienY(i));
    }
    return lo;
  }

  void resetAliens() {
    for (bool& a : alive_) a = true;
    left_ = kAliens;
    originX_ = 18.0f;
    originY_ = 22.0f;
    dir_ = 1;
  }

  void stepAliens() {
    float minX = 1e9f;
    float maxX = -1e9f;
    for (int i = 0; i < kAliens; ++i) {
      if (!alive_[i]) continue;
      const float x = static_cast<float>(alienX(i));
      minX = std::min(minX, x);
      maxX = std::max(maxX, x);
    }
    if (left_ == 0) return;

    const float next = originX_ + dir_ * kStepX;
    if (minX + dir_ * kStepX < 10.0f || maxX + dir_ * kStepX > w_ - 10.0f) {
      dir_ = -dir_;
      originY_ += kStepY;
    } else {
      originX_ = next;
    }
  }

  void hitAliens(ShellContext& ctx) {
    for (int i = 0; i < kAliens; ++i) {
      if (!alive_[i]) continue;
      const float ax = static_cast<float>(alienX(i));
      const float ay = static_cast<float>(alienY(i));
      if (std::fabs(shot_.x - ax) > kAlienW / 2.0f + 2.0f) continue;
      if (std::fabs(shot_.y - ay) > kAlienH / 2.0f + 4.0f) continue;
      alive_[i] = false;
      --left_;
      ++score_;
      shot_.active = false;
      cue(ctx, ExpressionKind::Chirp);
      if (left_ == 0) {
        ++wave_;
        resetAliens();
        cue(ctx, ExpressionKind::Celebrate);
      }
      return;
    }
  }

  void dropBomb() {
    int cols[kCols];
    int n = 0;
    for (int c = 0; c < kCols; ++c) {
      for (int r = kRows - 1; r >= 0; --r) {
        const int i = r * kCols + c;
        if (!alive_[i]) continue;
        cols[n++] = i;
        break;
      }
    }
    if (n == 0) return;
    const int i = cols[rng_.next() % static_cast<uint32_t>(n)];
    bomb_.active = true;
    bomb_.x = static_cast<float>(alienX(i));
    bomb_.y = static_cast<float>(alienY(i) + kAlienH / 2);
  }

  static constexpr int kCols = 6;
  static constexpr int kRows = 3;
  static constexpr int kAliens = kCols * kRows;
  static constexpr int kAlienW = 12;
  static constexpr int kAlienH = 8;
  static constexpr int kStrideX = 28;
  static constexpr int kStrideY = 16;
  static constexpr float kStepX = 4.0f;
  static constexpr float kStepY = 6.0f;
  static constexpr float kMoveSpeed = 140.0f;
  static constexpr float kShotSpeed = 180.0f;
  static constexpr float kBombSpeed = 90.0f;
  static constexpr float kBombChance = 0.012f;
  static constexpr DifficultyRamp kPaceRamp{520.0f, -4.0f, 180.0f, 0.0f};

  bool alive_[kAliens] = {};
  int left_ = kAliens;
  int wave_ = 0;
  float playerX_ = 0;
  float originX_ = 0;
  float originY_ = 0;
  int dir_ = 1;
  uint32_t moveAcc_ = 0;
  Bolt shot_;
  Bolt bomb_;
};

}  // namespace

TAMA_SCREEN_FACTORY(invaders, InvadersScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_INVADERS
