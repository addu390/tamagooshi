#include "brand.gen.h"
#if TAMA_GAME_CLIMB

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

struct Platform {
  float x;
  float y;
  int w;
};

class ClimbScreen : public ArcadeGameScreen {
 public:
  ClimbScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0xc11a6b21u); }
  const char* id() const override { return "game.climb"; }

  void onEnter(ShellContext& ctx) override {
    ArcadeGameScreen::onEnter(ctx);
    sensor_ = &ctx.sensor;
  }

  void onExit() override { sensor_ = nullptr; }

 protected:
  const char* title() const override { return "CLIMB"; }
  const char* readyHint() const override { return "TILT TO STEER"; }
  const char* runHint() const override { return nullptr; }
  const char* deadTitle() const override { return "FELL"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();
    for (const auto& p : plats_) {
      c.fillRoundRect(static_cast<int>(p.x - p.w / 2.0f), static_cast<int>(p.y), p.w, 4, 2,
                      theme::kFg);
    }
    const Expr e = vy_ < 0 ? Expr::Happy : Expr::Neutral;
    player(g, ctx, static_cast<int>(x_), static_cast<int>(y_), 24, e, 0, false);
  }

  void onReset() override {
    x_ = w_ / 2.0f;
    y_ = h_ - 60.0f;
    vy_ = kJump;
    climb_ = 0;
    const float gap = h_ / static_cast<float>(kPlatforms);
    for (int i = 0; i < kPlatforms; ++i) {
      plats_[i].y = h_ - 20 - i * gap;
      plats_[i].x = i == 0 ? x_ : rollX();
      plats_[i].w = kWideW;
    }
  }

  void step(ShellContext&) override {
    const float tilt = sensor_ ? sensor_->tiltX() : 0.0f;
    x_ += tilt * kSteerGain * kStepSec;
    if (x_ < 0) x_ += w_;
    if (x_ > w_) x_ -= w_;

    const float prevY = y_;
    vy_ += kGravity * kStepSec;
    y_ += vy_ * kStepSec;

    if (vy_ > 0) {
      for (const auto& p : plats_) {
        const bool crossed = prevY + kFootOffset <= p.y && y_ + kFootOffset >= p.y;
        if (crossed && x_ > p.x - p.w / 2.0f - 4 && x_ < p.x + p.w / 2.0f + 4) {
          vy_ = kJump;
          break;
        }
      }
    }

    if (y_ < h_ * 0.42f) {
      const float shift = h_ * 0.42f - y_;
      y_ += shift;
      climb_ += shift;
      score_ = static_cast<int>(climb_ / 24.0f);
      for (auto& p : plats_) {
        p.y += shift;
        if (p.y > h_) recycle(p);
      }
    }

    if (y_ - 12 > h_) die();
  }

 private:
  float rollX() { return 16.0f + rng_.unit() * (w_ - 32.0f); }

  void recycle(Platform& p) {
    float top = h_ * 2.0f;
    for (const auto& q : plats_)
      if (q.y < top) top = q.y;
    p.y = top - h_ / static_cast<float>(kPlatforms);
    p.x = rollX();
    p.w = static_cast<int>(kNarrowRamp.at(climb_ / 100.0f));
  }

  static constexpr int kPlatforms = 8;
  static constexpr int kWideW = 32;
  static constexpr int kFootOffset = 12;
  static constexpr float kGravity = 380.0f;
  static constexpr float kJump = -235.0f;
  static constexpr float kSteerGain = 300.0f;
  static constexpr DifficultyRamp kNarrowRamp{32.0f, -0.9f, 18.0f, 2.0f};

  ISensorSource* sensor_ = nullptr;
  Platform plats_[kPlatforms] = {};
  float x_ = 0;
  float y_ = 0;
  float vy_ = 0;
  float climb_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(climb, ClimbScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_CLIMB
