#include "brand.gen.h"
#if TAMA_GAME_PONG

#include <cmath>
#include <cstdio>

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

class PongScreen : public ArcadeGameScreen {
 public:
  PongScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0xb0119u); }
  const char* id() const override { return "game.pong"; }

 protected:
  const char* title() const override { return "PONG"; }
  const char* readyHint() const override { return "HOLD A / B TO MOVE"; }
  const char* runHint() const override { return "LEFT"; }
  const char* hintB() const override { return st_ == St::Run ? "RIGHT" : nullptr; }
  const char* deadTitle() const override { return foeScore_ > playerScore_ ? "LOSE" : "WIN"; }
  bool showScore() const override { return false; }
  int bannerCenterY() const override { return h_ / 2; }

  void renderWorld(Gfx& g, ShellContext&) override {
    auto& c = g.c();

    char score[12];
    std::snprintf(score, sizeof(score), "%d  %d", foeScore_, playerScore_);
    g.str(score, w_ / 2, 6, theme::kHi, typeface::title(), textdatum_t::top_center);

    for (int y = kTop; y < h_ - 20; y += 8) {
      c.fillRect(w_ / 2 - 1, y, 2, 4, theme::kDimmer);
    }

    c.fillRoundRect(static_cast<int>(foeX_) - kPaddleW / 2, foeY(), kPaddleW, 5, 2, theme::kDim);
    c.fillRoundRect(static_cast<int>(playerX_) - kPaddleW / 2, playerY(), kPaddleW, 5, 2,
                     theme::kHi);
    c.fillCircle(static_cast<int>(ballX_), static_cast<int>(ballY_), kBallR, theme::kFg);
  }

  void onReset() override {
    playerX_ = w_ / 2.0f;
    foeX_ = w_ / 2.0f;
    playerScore_ = 0;
    foeScore_ = 0;
    serve(-1);
  }

  void step(ShellContext& ctx) override {
    const float dt = kStepSec;

    if (ctx.buttons.held(0)) playerX_ -= kPaddleSpeed * dt;
    if (ctx.buttons.held(1)) playerX_ += kPaddleSpeed * dt;
    clampPaddle(playerX_);

    const float target = ballX_;
    const float maxStep = kFoeSpeed * dt;
    const float delta = target - foeX_;

    if (delta > maxStep) foeX_ += maxStep;
    else if (delta < -maxStep) foeX_ -= maxStep;
    else foeX_ = target;

    clampPaddle(foeX_);

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

    bounce(foeX_, foeY(), foeY() + 5, 1.0f, ctx);
    bounce(playerX_, playerY(), playerY() + 5, -1.0f, ctx);

    if (ballY_ < kTop - 4) {
      ++playerScore_;
      score_ = playerScore_;
      cue(ctx, ExpressionKind::Chirp);

      if (playerScore_ >= kWin) {
        die();
        return;
      }

      serve(1);
    }

    if (ballY_ > h_ + 4) {
      ++foeScore_;
      cue(ctx, ExpressionKind::Warn);

      if (foeScore_ >= kWin) {
        die();
        return;
      }

      serve(-1);
    }
  }

 private:
  static constexpr int kTop = 22;
  static constexpr int kPaddleW = 34;
  static constexpr int kBallR = 3;
  static constexpr int kWin = 5;
  static constexpr float kPaddleSpeed = 150.0f;
  static constexpr float kFoeSpeed = 95.0f;
  static constexpr float kSpeedBase = 110.0f;
  static constexpr DifficultyRamp kSpeedRamp{0.0f, 1.0f, 40.0f, 6.0f};

  int playerY() const { return h_ - 26; }
  int foeY() const { return kTop + 4; }
  float ballSpeed() const { return kSpeedBase + kSpeedRamp.at(elapsedSec()); }

  void clampPaddle(float& x) const {
    const float half = kPaddleW / 2.0f;
    if (x < half) x = half;
    if (x > w_ - half) x = w_ - half;
  }

  void serve(int dirY) {
    ballX_ = w_ / 2.0f;
    ballY_ = h_ * 0.5f;

    const float speed = ballSpeed();
    const float ang = 0.55f + rng_.unit() * 0.5f;
    vx_ = std::cos(ang) * speed * (rng_.unit() < 0.5f ? -1.0f : 1.0f);
    vy_ = std::sin(ang) * speed * (dirY < 0 ? -1.0f : 1.0f);
  }

  void bounce(float paddleX, int top, int bottom, float outVySign, ShellContext& ctx) {
    if (outVySign < 0) {
      if (vy_ <= 0) return;
      if (ballY_ + kBallR < top || ballY_ + kBallR > bottom + 2) return;
    } else {
      if (vy_ >= 0) return;
      if (ballY_ - kBallR > bottom || ballY_ - kBallR < top - 2) return;
    }

    if (std::fabs(ballX_ - paddleX) > kPaddleW / 2.0f + kBallR) return;

    const float offset = (ballX_ - paddleX) / (kPaddleW / 2.0f);
    const float speed = ballSpeed();
    vx_ = offset * speed * 0.75f;

    const float vy2 = speed * speed - vx_ * vx_;
    vy_ = outVySign * std::sqrt(vy2 > 1.0f ? vy2 : 1.0f);
    ballY_ = outVySign < 0 ? top - kBallR : bottom + kBallR;

    cue(ctx, ExpressionKind::Tick);
  }

  float playerX_ = 0;
  float foeX_ = 0;
  float ballX_ = 0;
  float ballY_ = 0;
  float vx_ = 0;
  float vy_ = 0;
  int playerScore_ = 0;
  int foeScore_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(pong, PongScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_PONG
