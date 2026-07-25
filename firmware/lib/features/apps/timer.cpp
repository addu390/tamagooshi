#include "brand.gen.h"
#if TAMA_APP_TIMER

#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kPresetsMin[] = {1, 3, 5, 10, 15, 25, 45, 60};
constexpr int kPresetCount = sizeof(kPresetsMin) / sizeof(kPresetsMin[0]);
constexpr int kDonePulses = 4;
constexpr uint32_t kPulseGapMs = 900;

enum class Phase { Idle, Run, Paused, Done };

class TimerScreen : public AppScreen {
 public:
  const char* id() const override { return "app.timer"; }

  void onEnter(ShellContext&) override {
    phase_ = Phase::Idle;
    preset_ = 2;
    remainingMs_ = durationMs();
    lastTick_ = 0;
    pulses_ = 0;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "TIMER");

    char mmss[6];
    const uint32_t s = (remainingMs_ + 999) / 1000;
    std::snprintf(mmss, sizeof(mmss), "%02u:%02u", s / 60, s % 60);
    const int timeY = L.top + (L.landscape ? 30 : 46);
    const lgfx::IFont* face = widgets::heroFont(g, "00:00", L.w - 12);
    const uint16_t col = phase_ == Phase::Done   ? theme::kHi
                         : phase_ == Phase::Run  ? theme::kFg
                                                 : theme::kDim;
    g.str(mmss, L.cx, timeY, col, face, textdatum_t::middle_center);

    const int barY = timeY + (L.landscape ? 24 : 36);
    const int barW = L.w - 40;
    const int fill = static_cast<int>(static_cast<uint64_t>(barW) * remainingMs_ / durationMs());
    g.c().drawRect(20, barY, barW, 5, theme::kDimmer);
    if (fill > 0) g.c().fillRect(20, barY, fill, 5, theme::kHi);

    const char* status = phase_ == Phase::Done     ? "TIME'S UP"
                         : phase_ == Phase::Paused ? "PAUSED"
                         : phase_ == Phase::Run    ? "RUNNING"
                                                   : "READY";
    widgets::pill(g, L.cx, barY + 14, status, typeface::micro(),
                  phase_ == Phase::Done ? theme::kHi : theme::kDim);

    widgets::hints(g, hintA(), hintB());
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) {
      switch (phase_) {
        case Phase::Idle:
          phase_ = Phase::Run;
          break;
        case Phase::Run:
          phase_ = Phase::Paused;
          break;
        case Phase::Paused:
          phase_ = Phase::Run;
          break;
        case Phase::Done:
          reset();
          break;
      }
      cue(ctx, ExpressionKind::Blink);
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      if (phase_ == Phase::Idle) {
        preset_ = (preset_ + 1) % kPresetCount;
        remainingMs_ = durationMs();
      } else {
        reset();
      }
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (lastTick_ == 0) lastTick_ = nowMs;
    const uint32_t dt = nowMs - lastTick_;
    lastTick_ = nowMs;

    if (phase_ == Phase::Run) {
      if (remainingMs_ <= dt) {
        remainingMs_ = 0;
        phase_ = Phase::Done;
        pulses_ = 0;
        cue(ctx, ExpressionKind::Celebrate);
      } else {
        remainingMs_ -= dt;
      }
      return anim_.due(nowMs, 250) ? Transition::redraw() : Transition::none();
    }

    if (phase_ == Phase::Done && pulses_ < kDonePulses && anim_.due(nowMs, kPulseGapMs)) {
      ++pulses_;
      cue(ctx, ExpressionKind::Haptic);
      return Transition::redraw();
    }
    return Transition::none();
  }

 private:
  uint32_t durationMs() const { return static_cast<uint32_t>(kPresetsMin[preset_]) * 60000u; }

  void reset() {
    phase_ = Phase::Idle;
    remainingMs_ = durationMs();
  }

  const char* hintA() const {
    switch (phase_) {
      case Phase::Idle:
        return "START";
      case Phase::Run:
        return "PAUSE";
      case Phase::Paused:
        return "RESUME";
      case Phase::Done:
        return "OK";
    }
    return "";
  }

  const char* hintB() const { return phase_ == Phase::Idle ? "PRESET" : "RESET"; }

  AnimClock anim_;
  Phase phase_ = Phase::Idle;
  int preset_ = 2;
  uint32_t remainingMs_ = 0;
  uint32_t lastTick_ = 0;
  int pulses_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(timer, TimerScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_TIMER
