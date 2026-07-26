#include "brand.gen.h"
#if TAMA_APP_METRONOME

#include <cmath>
#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kBpms[] = {60, 72, 84, 96, 108, 120, 132, 144, 160, 180, 200};
constexpr int kBpmCount = sizeof(kBpms) / sizeof(kBpms[0]);
constexpr int kBeatsPerBar = 4;

class MetronomeScreen : public AppScreen {
 public:
  const char* id() const override { return "app.metronome"; }

  void onEnter(ShellContext&) override {
    running_ = false;
    sel_ = 5;
    beat_ = 0;
    lastBeat_ = 0;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "METRONOME");

    char bpm[8];
    std::snprintf(bpm, sizeof(bpm), "%d", kBpms[sel_]);
    const int bpmY = L.top + (L.landscape ? 24 : 36);
    g.str(bpm, L.cx, bpmY, running_ ? theme::kFg : theme::kDim,
          widgets::heroFont(g, "200", L.w / 2), textdatum_t::middle_center);
    g.str("BPM", L.cx, bpmY + 22, theme::kDimmer, typeface::micro(), textdatum_t::middle_center);

    const int laneY = bpmY + (L.landscape ? 40 : 56);
    const int amp = L.w / 2 - 28;
    auto& c = g.c();
    c.drawFastHLine(L.cx - amp, laneY, 2 * amp, theme::kDimmer);
    const int x = L.cx + static_cast<int>(amp * pendulum());
    const bool accent = beat_ % kBeatsPerBar == 0;
    c.fillCircle(x, laneY, 7, running_ ? (accent ? theme::kHi : theme::kFg) : theme::kDim);

    widgets::dots(g, L.cx, laneY + 18, kBeatsPerBar, running_ ? beat_ % kBeatsPerBar : -1);

    widgets::hints(g, running_ ? "STOP" : "START", "TEMPO");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Select) {
      running_ = !running_;
      beat_ = 0;
      lastBeat_ = 0;
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      sel_ = (sel_ + 1) % kBpmCount;
      lastBeat_ = 0;
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    AppScreen::tick(ctx, nowMs);
    if (!running_) return Transition::none();

    if (lastBeat_ == 0) lastBeat_ = nowMs;
    if (nowMs - lastBeat_ >= beatMs()) {
      lastBeat_ += beatMs();
      ++beat_;
      const bool accent = beat_ % kBeatsPerBar == 0;
      cue(ctx, ExpressionKind::Tick, accent ? 100 : 55);
    }
    return Transition::redraw();
  }

 private:
  uint32_t beatMs() const { return 60000u / static_cast<uint32_t>(kBpms[sel_]); }

  float pendulum() const {
    if (!running_ || lastBeat_ == 0) return 0.0f;
    const float t = static_cast<float>(now() - lastBeat_) / beatMs();
    const float phase = (beat_ % 2 == 0) ? t : 1.0f - t;
    return 2.0f * phase - 1.0f;
  }

  bool running_ = false;
  int sel_ = 5;
  int beat_ = 0;
  uint32_t lastBeat_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(metronome, MetronomeScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_METRONOME
