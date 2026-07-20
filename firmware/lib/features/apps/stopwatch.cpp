#include "brand.gen.h"
#if TAMA_APP_STOPWATCH

#include <algorithm>
#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

class StopwatchScreen : public AppScreen {
 public:
  const char* id() const override { return "app.stopwatch"; }

  void onEnter(ShellContext&) override {
    running_ = false;
    base_ = 0;
    startedAt_ = 0;
    now_ = 0;
    lapCount_ = 0;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "STOPWATCH");

    const uint32_t ms = elapsed();
    char mmss[6];
    std::snprintf(mmss, sizeof(mmss), "%02u:%02u", (ms / 60000) % 100, (ms / 1000) % 60);
    const int timeY = L.top + (L.landscape ? 26 : 40);
    const lgfx::IFont* face = widgets::heroFont(g, "00:00", L.w - 12);
    g.str(mmss, L.cx, timeY, running_ ? theme::kFg : theme::kDim, face, textdatum_t::middle_center);

    char cs[4];
    std::snprintf(cs, sizeof(cs), ".%02u", (ms / 10) % 100);
    if (L.landscape) {
      const int halfW = g.textWidth(mmss, face) / 2;
      g.str(cs, L.cx + halfW + 4, timeY + 8, theme::kDim, typeface::body(),
            textdatum_t::middle_left);
    } else {
      g.str(cs, L.cx, timeY + 30, theme::kDim, typeface::body(), textdatum_t::middle_center);
    }

    int lapY = timeY + (L.landscape ? 26 : 46);
    const int lapW = std::min(L.w - 24, 120);
    for (int i = 0; i < lapCount_; ++i) {
      char lap[4];
      std::snprintf(lap, sizeof(lap), "L%d", i + 1);
      char t[12];
      std::snprintf(t, sizeof(t), "%02u:%02u.%02u", (laps_[i] / 60000) % 100,
                    (laps_[i] / 1000) % 60, (laps_[i] / 10) % 100);
      g.str(lap, L.cx - lapW / 2, lapY, theme::kDimmer, typeface::micro(), textdatum_t::top_left);
      g.str(t, L.cx + lapW / 2, lapY, theme::kDim, typeface::micro(), textdatum_t::top_right);
      lapY += 14;
    }

    widgets::hints(g, running_ ? "STOP" : "START", running_ ? "LAP" : "RESET");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Select) {
      if (running_) {
        base_ = elapsed();
        running_ = false;
      } else {
        startedAt_ = now_;
        running_ = true;
      }
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      if (running_) {
        if (lapCount_ < kMaxLaps) laps_[lapCount_++] = elapsed();
      } else {
        base_ = 0;
        lapCount_ = 0;
      }
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    now_ = nowMs;
    if (!running_) return Transition::none();
    if (!anim_.due(nowMs, 33)) return Transition::none();
    return Transition::redraw();
  }

 private:
  static constexpr int kMaxLaps = 3;

  uint32_t elapsed() const { return running_ ? base_ + (now_ - startedAt_) : base_; }

  AnimClock anim_;
  bool running_ = false;
  uint32_t base_ = 0;
  uint32_t startedAt_ = 0;
  uint32_t now_ = 0;
  uint32_t laps_[kMaxLaps] = {};
  int lapCount_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(stopwatch, StopwatchScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_STOPWATCH
