#include "brand.gen.h"
#if TAMA_GAME_HUSH

#include <algorithm>

#include "arcade.h"
#include "audio.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

enum class Guard { Asleep, Waking, Awake };

class HushScreen : public ArcadeGameScreen {
 public:
  HushScreen() : ArcadeGameScreen(OrientationPref::Landscape) { rng_.seed(0x4a51105du); }
  const char* id() const override { return "game.hush"; }

  void onEnter(ShellContext& ctx) override {
    ArcadeGameScreen::onEnter(ctx);
    mic_ = &ctx.mic;
    mic_->begin();
  }

  void onExit() override {
    if (mic_) {
      mic_->end();
      mic_ = nullptr;
    }
  }

 protected:
  const char* title() const override { return "HUSH"; }
  const char* readyHint() const override { return "SNEAK WHILE QUIET"; }
  const char* runHint() const override { return nullptr; }
  const char* deadTitle() const override { return "CAUGHT"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();

    const int alarmW = static_cast<int>((w_ - 24) * alarm_);
    c.drawRect(12, 20, w_ - 24, 6, theme::kDim);
    if (alarmW > 0) c.fillRect(12, 20, alarmW, 6, alarm_ > 0.7f ? theme::kCrit : theme::kWarn);

    drawGuard(g, w_ - 34, h_ / 2 - 12);

    const bool loud = meter_.loud();
    const Expr e = loud ? Expr::Worried : Expr::Happy;
    player(g, ctx, static_cast<int>(x_), h_ - 32, 26, e, 0, true);

    const int len = std::min(w_, w_ * meter_.level() / (meter_.threshold() * 2));
    c.fillRect(0, h_ - 3, w_, 3, theme::kBg);
    if (len > 0) c.fillRect(0, h_ - 3, len, 3, loud ? theme::kHi : theme::kDim);
  }

  void onReset() override {
    x_ = 20;
    alarm_ = 0;
    guard_ = Guard::Asleep;
    guardMs_ = 0;
    guardSpan_ = 2500;
  }

  void step(ShellContext& ctx) override {
    meter_.sample(mic_);
    const bool loud = meter_.loud();

    advanceGuard(ctx);

    if (loud) {
      if (guard_ == Guard::Awake) return die();
      alarm_ += kAlarmRate * kStepSec * (guard_ == Guard::Waking ? 2.0f : 1.0f);
      if (alarm_ >= 1.0f) return die();
    } else {
      alarm_ -= kAlarmDecay * kStepSec;
      if (alarm_ < 0) alarm_ = 0;
      x_ += kWalkRate * kStepSec;
      if (x_ > w_ - 52) {
        x_ = 20;
        ++score_;
        cue(ctx, ExpressionKind::Chirp);
      }
    }
  }

 private:
  void advanceGuard(ShellContext& ctx) {
    guardMs_ += kStepMs;
    if (guardMs_ < guardSpan_) return;
    guardMs_ = 0;
    switch (guard_) {
      case Guard::Asleep:
        guard_ = Guard::Waking;
        guardSpan_ = 700;
        cue(ctx, ExpressionKind::Warn);
        break;
      case Guard::Waking:
        guard_ = Guard::Awake;
        guardSpan_ = 900 + rng_.next() % 1300u;
        break;
      case Guard::Awake:
        guard_ = Guard::Asleep;
        guardSpan_ = static_cast<uint32_t>(kSleepRamp.at(elapsedSec())) + rng_.next() % 1500u;
        break;
    }
  }

  void drawGuard(Gfx& g, int cx, int cy) {
    auto& c = g.c();
    const uint16_t col = guard_ == Guard::Awake   ? theme::kCrit
                         : guard_ == Guard::Waking ? theme::kWarn
                                                   : theme::kDim;
    c.fillCircle(cx, cy, 12, theme::kDimmer);
    c.drawCircle(cx, cy, 12, col);
    if (guard_ == Guard::Asleep) {
      c.drawFastHLine(cx - 7, cy - 2, 5, col);
      c.drawFastHLine(cx + 2, cy - 2, 5, col);
      g.str("z", cx + 14, cy - 14, theme::kDim, typeface::micro(), textdatum_t::middle_center);
    } else {
      c.fillCircle(cx - 5, cy - 2, 2, col);
      c.fillCircle(cx + 5, cy - 2, 2, col);
    }
  }

  static constexpr int kMinDelta = 200;
  static constexpr float kWalkRate = 30.0f;
  static constexpr float kAlarmRate = 0.55f;
  static constexpr float kAlarmDecay = 0.22f;
  static constexpr DifficultyRamp kSleepRamp{2600.0f, -60.0f, 1200.0f, 5.0f};

  IMicSource* mic_ = nullptr;
  audio::LevelMeter meter_{kMinDelta};
  Guard guard_ = Guard::Asleep;
  uint32_t guardMs_ = 0;
  uint32_t guardSpan_ = 2500;
  float x_ = 20;
  float alarm_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(hush, HushScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_HUSH
