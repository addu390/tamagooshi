#include "brand.gen.h"
#if TAMA_APP_BREATH

#include <algorithm>
#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

enum class Phase : uint8_t { Idle, Inhale, HoldFull, Exhale, HoldEmpty };

struct Step {
  Phase phase;
  uint16_t secs;
};

struct Preset {
  const char* name;
  const Step* steps;
  int count;
};

constexpr Step kCalm[] = {{Phase::Inhale, 4}, {Phase::Exhale, 4}};
constexpr Step kBox[] = {{Phase::Inhale, 4},
                         {Phase::HoldFull, 4},
                         {Phase::Exhale, 4},
                         {Phase::HoldEmpty, 4}};
constexpr Step kRelax[] = {{Phase::Inhale, 4}, {Phase::HoldFull, 7}, {Phase::Exhale, 8}};

constexpr Preset kPresets[] = {
    {"CALM", kCalm, 2},
    {"BOX", kBox, 4},
    {"RELAX", kRelax, 3},
};
constexpr int kPresetCount = sizeof(kPresets) / sizeof(kPresets[0]);

const char* phaseLabel(Phase p) {
  switch (p) {
    case Phase::Inhale:
      return "INHALE";
    case Phase::HoldFull:
      return "HOLD";
    case Phase::Exhale:
      return "EXHALE";
    case Phase::HoldEmpty:
      return "REST";
    case Phase::Idle:
      return "READY";
  }
  return "";
}

class BreathScreen : public AppScreen {
 public:
  const char* id() const override { return "app.breath"; }
  OrientationPref orientation() const override { return OrientationPref::Portrait; }

  void onEnter(ShellContext&) override {
    running_ = false;
    preset_ = 0;
    step_ = 0;
    phaseMs_ = 0;
    lastTick_ = 0;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "BREATH");
    const Preset& preset = kPresets[preset_];
    const Phase phase = running_ ? preset.steps[step_].phase : Phase::Idle;

    widgets::pill(g, L.cx, L.top + 18, running_ ? phaseLabel(phase) : preset.name,
                  typeface::micro(), running_ ? theme::kHi : theme::kDim);

    const int cx = L.cx;
    const int cy = L.top + (L.contentH() / 2) - 4;
    const int minR = 16;
    const int maxR = std::min(L.w / 2 - 12, 46);
    const float fill = fillAmount(phase);
    const int r = minR + static_cast<int>((maxR - minR) * fill);

    auto& c = g.c();
    c.drawCircle(cx, cy, maxR, theme::kDimmer);
    if (r > 0) c.fillCircle(cx, cy, r, theme::kHi);
    c.drawCircle(cx, cy, r, theme::kFg);

    if (running_) {
      const uint32_t total = stepMs();
      const uint32_t left = total > phaseMs_ ? total - phaseMs_ : 0;
      char secs[8];
      std::snprintf(secs, sizeof(secs), "%u", (left + 999) / 1000);
      g.str(secs, cx, cy, theme::kBg, typeface::title(), textdatum_t::middle_center);
    } else {
      g.str("BREATHE", cx, cy, theme::kDim, typeface::micro(), textdatum_t::middle_center);
    }

    widgets::hints(g, running_ ? "PAUSE" : "START", running_ ? "RESET" : "PRESET");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) {
      running_ = !running_;
      if (running_) cue(ctx, ExpressionKind::Blink);
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      if (running_) {
        running_ = false;
        step_ = 0;
        phaseMs_ = 0;
      } else {
        preset_ = (preset_ + 1) % kPresetCount;
      }
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (lastTick_ == 0) lastTick_ = nowMs;
    const uint32_t dt = nowMs - lastTick_;
    lastTick_ = nowMs;

    if (!running_) return Transition::none();

    phaseMs_ += dt;
    if (phaseMs_ >= stepMs()) {
      phaseMs_ = 0;
      step_ = (step_ + 1) % kPresets[preset_].count;
      cue(ctx, ExpressionKind::Blink);
    }

    if (!anim_.due(nowMs, 33)) return Transition::none();
    return Transition::redraw();
  }

 private:
  uint32_t stepMs() const {
    return static_cast<uint32_t>(kPresets[preset_].steps[step_].secs) * 1000u;
  }

  float fillAmount(Phase phase) const {
    if (!running_) return 0.35f;
    const float t =
        stepMs() == 0 ? 0.f : std::min(1.f, static_cast<float>(phaseMs_) / static_cast<float>(stepMs()));
    switch (phase) {
      case Phase::Inhale:
        return t;
      case Phase::Exhale:
        return 1.f - t;
      case Phase::HoldFull:
        return 1.f;
      case Phase::HoldEmpty:
        return 0.f;
      case Phase::Idle:
        return 0.35f;
    }
    return 0.35f;
  }

  AnimClock anim_;
  bool running_ = false;
  int preset_ = 0;
  int step_ = 0;
  uint32_t phaseMs_ = 0;
  uint32_t lastTick_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(breath, BreathScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_BREATH
