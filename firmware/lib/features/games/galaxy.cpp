#include "brand.gen.h"
#if TAMA_GAME_GALAXY

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

class GalaxyScreen : public ArcadeGameScreen {
 public:
  GalaxyScreen() : ArcadeGameScreen(OrientationPref::Landscape) { rng_.seed(0x9a1c0f37u); }
  const char* id() const override { return "game.galaxy"; }

  void onEnter(ShellContext& ctx) override {
    ArcadeGameScreen::onEnter(ctx);
    sensor_ = &ctx.sensor;
  }

  void onExit() override { sensor_ = nullptr; }

 protected:
  const char* title() const override { return "GALAXY"; }
  const char* readyHint() const override { return "TILT TO DODGE"; }
  const char* runHint() const override { return "TILT!"; }
  const char* deadTitle() const override { return "CRASHED"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();

    for (const auto& m : rocks_) {
      if (!m.active) continue;
      const int x = static_cast<int>(m.x);
      const int y = static_cast<int>(m.y);
      c.fillCircle(x, y, kRockR, theme::kFg);
      c.drawCircle(x, y, kRockR, theme::kDim);
    }

    player(g, ctx, static_cast<int>(playerX_), kPlayerY, kPlayerSz, Expr::Neutral, 0, false);
  }

  void onReset() override {
    playerX_ = w_ / 2.0f;
    for (auto& m : rocks_) m.active = false;
  }

  void step(ShellContext&) override {
    const float tilt = sensor_ ? sensor_->tiltY() : 0.0f;
    playerX_ += tilt * kMoveGain;
    const float half = kPlayerSz / 2.0f;
    if (playerX_ < half) playerX_ = half;
    if (playerX_ > w_ - half) playerX_ = w_ - half;

    const float d = kRamp.at(elapsedSec());
    if (rng_.unit() < kSpawnBase + d * kSpawnGain) spawn(d);

    for (auto& m : rocks_) {
      if (!m.active) continue;
      m.y += m.vy;
      if (m.y - kRockR > h_) {
        m.active = false;
        ++score_;
        continue;
      }
      if (hits(m)) return die();
    }
  }

 private:
  struct Rock {
    float x = 0, y = 0, vy = 0;
    bool active = false;
  };

  void spawn(float d) {
    for (auto& m : rocks_) {
      if (m.active) continue;
      m.active = true;
      float x;
      if (rng_.unit() < kAimFrac) {
        x = playerX_ + (rng_.unit() * 2.0f - 1.0f) * kAimSpread;
      } else {
        x = kRockR + rng_.unit() * (w_ - 2 * kRockR);
      }
      const float lo = kRockR;
      const float hi = w_ - kRockR;
      m.x = x < lo ? lo : (x > hi ? hi : x);
      m.y = -static_cast<float>(kRockR);
      m.vy = kFallMin + d * kFallGain + rng_.unit() * kFallJitter * d;
      return;
    }
  }

  bool hits(const Rock& m) const {
    const float half = kPlayerSz / 2.0f;
    const float dx = m.x - playerX_;
    const float dy = m.y - kPlayerY;
    return dx > -(half + kRockR) && dx < (half + kRockR) && dy > -(half + kRockR) &&
           dy < (half + kRockR);
  }

  static constexpr int kPlayerSz = 22;
  static constexpr int kPlayerY = 118;
  static constexpr int kRockR = 6;
  static constexpr float kMoveGain = 9.0f;
  static constexpr DifficultyRamp kRamp{0.0f, 1.0f / 50.0f, 1.0f, 2.0f};
  static constexpr float kFallMin = 0.55f;
  static constexpr float kFallGain = 1.7f;
  static constexpr float kFallJitter = 0.6f;
  static constexpr float kSpawnBase = 0.018f;
  static constexpr float kSpawnGain = 0.05f;
  static constexpr float kAimFrac = 0.7f;
  static constexpr float kAimSpread = 95.0f;

  ISensorSource* sensor_ = nullptr;
  float playerX_ = 120;
  Rock rocks_[6];
};

}  // namespace

TAMA_SCREEN_FACTORY(galaxy, GalaxyScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_GALAXY
