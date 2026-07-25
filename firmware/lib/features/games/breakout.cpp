#include "brand.gen.h"
#if TAMA_GAME_BREAKOUT

#include <cmath>

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

class BreakoutScreen : public ArcadeGameScreen {
 public:
  BreakoutScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0xb41cba11u); }
  const char* id() const override { return "game.breakout"; }

 protected:
  const char* title() const override { return "BREAKOUT"; }
  const char* readyHint() const override { return "HOLD A / B TO MOVE"; }
  const char* runHint() const override { return "LEFT"; }
  const char* hintB() const override { return st_ == St::Run ? "RIGHT" : nullptr; }

  void renderWorld(Gfx& g, ShellContext&) override {
    auto& c = g.c();

    for (int i = 0; i < kBricks; ++i) {
      if (!alive_[i]) continue;
      const auto [bx, by] = brickPx(i);
      c.fillRect(bx + 1, by + 1, brickW() - 2, kBrickH - 2, theme::kDim);
      c.drawRect(bx + 1, by + 1, brickW() - 2, kBrickH - 2, theme::kFg);
    }

    const int py = paddleY();
    c.fillRoundRect(static_cast<int>(paddleX_) - kPaddleW / 2, py, kPaddleW, 5, 2, theme::kHi);
    c.fillCircle(static_cast<int>(ballX_), static_cast<int>(ballY_), kBallR, theme::kFg);
  }

  void onReset() override {
    paddleX_ = w_ / 2.0f;
    wave_ = 0;
    resetBricks();
    serve();
  }

  void step(ShellContext& ctx) override {
    const float dt = kStepSec;
    if (ctx.buttons.held(0)) paddleX_ -= kPaddleSpeed * dt;
    if (ctx.buttons.held(1)) paddleX_ += kPaddleSpeed * dt;
    const float halfP = kPaddleW / 2.0f;
    if (paddleX_ < halfP) paddleX_ = halfP;
    if (paddleX_ > w_ - halfP) paddleX_ = w_ - halfP;

    ballX_ += vx_ * dt;
    ballY_ += vy_ * dt;

    if (ballX_ < kBallR) {
      ballX_ = kBallR;
      vx_ = -vx_;
    }
    if (ballX_ > w_ - kBallR) {
      ballX_ = w_ - kBallR;
      vx_ = -vx_;
    }
    if (ballY_ < kTop + kBallR) {
      ballY_ = kTop + kBallR;
      vy_ = -vy_;
    }

    const int py = paddleY();
    if (vy_ > 0 && ballY_ + kBallR >= py && ballY_ + kBallR <= py + 6 &&
        std::fabs(ballX_ - paddleX_) <= kPaddleW / 2.0f + kBallR) {
      const float offset = (ballX_ - paddleX_) / (kPaddleW / 2.0f);
      const float speed = ballSpeed();
      vx_ = offset * speed * 0.75f;
      vy_ = -std::sqrt(speed * speed - vx_ * vx_);
      ballY_ = py - kBallR;
    }

    hitBricks(ctx);

    if (ballY_ - kBallR > h_) die();
  }

 private:
  int brickW() const { return (w_ - 8) / kCols; }
  int paddleY() const { return h_ - 26; }
  float ballSpeed() const { return kSpeedBase + wave_ * 14.0f + kSpeedRamp.at(elapsedSec()); }

  std::pair<int, int> brickPx(int i) const {
    return {4 + (i % kCols) * brickW(), kTop + 8 + (i / kCols) * kBrickH};
  }

  void resetBricks() {
    for (bool& b : alive_) b = true;
    left_ = kBricks;
  }

  void serve() {
    ballX_ = w_ / 2.0f;
    ballY_ = h_ * 0.6f;
    const float speed = ballSpeed();
    const float ang = 0.6f + rng_.unit() * 0.5f;
    vx_ = std::cos(ang) * speed * (rng_.unit() < 0.5f ? -1.0f : 1.0f);
    vy_ = -std::sin(ang) * speed;
  }

  void hitBricks(ShellContext& ctx) {
    for (int i = 0; i < kBricks; ++i) {
      if (!alive_[i]) continue;
      const auto [bx, by] = brickPx(i);
      if (ballX_ + kBallR < bx || ballX_ - kBallR > bx + brickW() || ballY_ + kBallR < by ||
          ballY_ - kBallR > by + kBrickH) {
        continue;
      }
      alive_[i] = false;
      --left_;
      ++score_;
      vy_ = -vy_;
      cue(ctx, ExpressionKind::Chirp);
      if (left_ == 0) {
        ++wave_;
        resetBricks();
        serve();
      }
      return;
    }
  }

  static constexpr int kCols = 6;
  static constexpr int kRows = 5;
  static constexpr int kBricks = kCols * kRows;
  static constexpr int kBrickH = 10;
  static constexpr int kTop = 20;
  static constexpr int kPaddleW = 34;
  static constexpr int kBallR = 3;
  static constexpr float kPaddleSpeed = 150.0f;
  static constexpr float kSpeedBase = 105.0f;
  static constexpr DifficultyRamp kSpeedRamp{0.0f, 1.2f, 45.0f, 8.0f};

  bool alive_[kBricks] = {};
  int left_ = kBricks;
  int wave_ = 0;
  float paddleX_ = 0;
  float ballX_ = 0;
  float ballY_ = 0;
  float vx_ = 0;
  float vy_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(breakout, BreakoutScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_BREAKOUT
