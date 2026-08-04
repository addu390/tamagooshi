#include "brand.gen.h"
#if TAMA_APP_ALARM

#include <ctime>
#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kDonePulses = 8;
constexpr uint32_t kPulseGapMs = 700;
constexpr int kStepMin = 5;
constexpr int kSnoozeMin = 5;

enum class Phase { Idle, Armed, Ringing };

class AlarmScreen : public AppScreen {
 public:
  const char* id() const override { return "app.alarm"; }

  void onEnter(ShellContext& ctx) override {
    phase_ = Phase::Idle;
    pulses_ = 0;
    lastTick_ = 0;
    firedMinute_ = -1;

    if (hour_ < 0) {
      const auto tm = localTm(ctx);
      hour_ = tm.tm_hour;
      minute_ = (tm.tm_min / kStepMin) * kStepMin;
    }
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "ALARM");

    char hhmm[6];
    std::snprintf(hhmm, sizeof(hhmm), "%02d:%02d", hour_, minute_);
    const int timeY = L.top + (L.landscape ? 30 : 46);
    const lgfx::IFont* face = widgets::heroFont(g, "00:00", L.w - 12);
    const uint16_t col = phase_ == Phase::Ringing ? theme::kHi
                         : phase_ == Phase::Armed   ? theme::kFg
                                                    : theme::kDim;
    g.str(hhmm, L.cx, timeY, col, face, textdatum_t::middle_center);

    const char* status = phase_ == Phase::Ringing ? "RINGING"
                         : phase_ == Phase::Armed   ? "ARMED"
                                                    : "SET TIME";
    widgets::pill(g, L.cx, timeY + (L.landscape ? 24 : 36), status, typeface::micro(),
                  phase_ == Phase::Ringing ? theme::kHi : theme::kDim);

    if (phase_ == Phase::Armed) {
      const auto tm = localTm(ctx);
      char now[16];
      std::snprintf(now, sizeof(now), "NOW %02d:%02d", tm.tm_hour, tm.tm_min);
      g.str(now, L.cx, L.bottom - 30, theme::kDimmer, typeface::micro(),
            textdatum_t::bottom_center);
    }

    widgets::hints(g, hintA(), hintB());
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) {
      switch (phase_) {
        case Phase::Idle:
          phase_ = Phase::Armed;
          firedMinute_ = -1;
          break;
        case Phase::Armed:
          phase_ = Phase::Idle;
          break;
        case Phase::Ringing:
          phase_ = Phase::Idle;
          pulses_ = 0;
          break;
      }
      cue(ctx, ExpressionKind::Blink);
      return Transition::redraw();
    }

    if (intent == Intent::Next) {
      if (phase_ == Phase::Idle) {
        bump(kStepMin);
        return Transition::redraw();
      }

      if (phase_ == Phase::Ringing) {
        const auto tm = localTm(ctx);
        hour_ = tm.tm_hour;
        minute_ = tm.tm_min;
        bump(kSnoozeMin);
        phase_ = Phase::Armed;
        firedMinute_ = -1;
        pulses_ = 0;
        cue(ctx, ExpressionKind::Blink);
        return Transition::redraw();
      }
    }

    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (lastTick_ == 0) lastTick_ = nowMs;
    lastTick_ = nowMs;

    if (phase_ == Phase::Armed) {
      const auto tm = localTm(ctx);
      const int stamp = tm.tm_hour * 60 + tm.tm_min;
      const int target = hour_ * 60 + minute_;

      if (stamp == target && stamp != firedMinute_) {
        firedMinute_ = stamp;
        phase_ = Phase::Ringing;
        pulses_ = 0;
        cue(ctx, ExpressionKind::Celebrate);
        return Transition::redraw();
      }

      return anim_.due(nowMs, 500) ? Transition::redraw() : Transition::none();
    }

    if (phase_ == Phase::Ringing && pulses_ < kDonePulses && anim_.due(nowMs, kPulseGapMs)) {
      ++pulses_;
      cue(ctx, ExpressionKind::Tick);
      return Transition::redraw();
    }

    return Transition::none();
  }

 private:
  std::tm localTm(ShellContext& ctx) const {
    const std::time_t local = std::time(nullptr) + ctx.state.tz_offset_min * 60;
    std::tm tm{};
    gmtime_r(&local, &tm);
    return tm;
  }

  void bump(int mins) {
    minute_ += mins;
    hour_ = (hour_ + minute_ / 60) % 24;
    minute_ %= 60;
  }

  const char* hintA() const {
    switch (phase_) {
      case Phase::Idle:
        return "ARM";
      case Phase::Armed:
        return "DISARM";
      case Phase::Ringing:
        return "DISMISS";
    }
    return "";
  }

  const char* hintB() const {
    switch (phase_) {
      case Phase::Idle:
        return "+5 MIN";
      case Phase::Armed:
        return nullptr;
      case Phase::Ringing:
        return "SNOOZE";
    }
    return nullptr;
  }

  AnimClock anim_;
  Phase phase_ = Phase::Idle;
  int hour_ = -1;
  int minute_ = 0;
  uint32_t lastTick_ = 0;
  int pulses_ = 0;
  int firedMinute_ = -1;
};

}  // namespace

TAMA_SCREEN_FACTORY(alarm, AlarmScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_ALARM
