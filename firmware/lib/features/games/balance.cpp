#include "brand.gen.h"
#if TAMA_GAME_BALANCE

#include <cmath>

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

class BalanceScreen : public ArcadeGameScreen {
 public:
  BalanceScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0xba1a4ce5u); }
  const char* id() const override { return "game.balance"; }

  void onEnter(ShellContext& ctx) override {
    ArcadeGameScreen::onEnter(ctx);
    sensor_ = &ctx.sensor;
  }

  void onExit() override { sensor_ = nullptr; }

 protected:
  const char* title() const override { return "BALANCE"; }
  const char* readyHint() const override { return "TILT TO STAY ON"; }
  const char* runHint() const override { return nullptr; }
  const char* deadTitle() const override { return "FELL OFF"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();
    const int cy = h_ / 2 + 24;
    const int half = beamHalf();

    c.fillTriangle(w_ / 2, cy + 4, w_ / 2 - 12, cy + 26, w_ / 2 + 12, cy + 26, theme::kDimmer);
    c.fillRoundRect(w_ / 2 - half, cy, 2 * half, 4, 2, theme::kFg);
    c.drawFastVLine(w_ / 2, cy - 4, 4, theme::kDim);

    drawWind(g, w_ / 2, h_ / 4);

    const int px = w_ / 2 + static_cast<int>(pos_);
    const float edge = std::fabs(pos_) / half;
    const Expr e = edge > 0.7f ? Expr::Worried : Expr::Neutral;
    player(g, ctx, px, cy - 14, 26, e, 0, false);
  }

  void onReset() override {
    pos_ = 0;
    vel_ = 0;
    wind_ = 0;
    gust_ = 0;
    scoreAcc_ = 0;
  }

  void step(ShellContext&) override {
    const float sec = elapsedSec();
    const float amp = kWindRamp.at(sec);

    if (rng_.unit() < 0.01f) gust_ = (rng_.unit() * 2.0f - 1.0f) * amp;
    wind_ += (gust_ - wind_) * 0.02f;

    const float tilt = sensor_ ? sensor_->tiltX() : 0.0f;
    vel_ += (tilt * kTiltGain + wind_) * kStepSec;
    vel_ *= kDamping;
    pos_ += vel_ * kStepSec;

    scoreAcc_ += kStepSec;
    if (scoreAcc_ >= 1.0f) {
      scoreAcc_ -= 1.0f;
      ++score_;
    }

    if (std::fabs(pos_) > beamHalf()) die();
  }

 private:
  int beamHalf() const { return (w_ - 30) / 2; }

  void drawWind(Gfx& g, int cx, int y) {
    if (std::fabs(wind_) < 3.0f) return;
    auto& c = g.c();
    const int len = static_cast<int>(std::fabs(wind_) / 4.0f) + 8;
    const int dir = wind_ > 0 ? 1 : -1;
    const int tip = cx + dir * len / 2;
    c.drawFastHLine(cx - dir * len / 2, y, dir * len, theme::kWarn);
    c.drawLine(tip, y, tip - dir * 4, y - 3, theme::kWarn);
    c.drawLine(tip, y, tip - dir * 4, y + 3, theme::kWarn);
  }

  static constexpr float kTiltGain = 260.0f;
  static constexpr float kDamping = 0.995f;
  static constexpr DifficultyRamp kWindRamp{18.0f, 2.2f, 90.0f, 4.0f};

  ISensorSource* sensor_ = nullptr;
  float pos_ = 0;
  float vel_ = 0;
  float wind_ = 0;
  float gust_ = 0;
  float scoreAcc_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(balance, BalanceScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_BALANCE
