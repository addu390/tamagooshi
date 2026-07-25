#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

class MetricsScreen : public AppScreen {
 public:
  const char* id() const override { return "metrics"; }
  void onEnter(ShellContext&) override {
    idx_ = 0;
    primed_ = false;
    confirm_.cancel();
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto& s = ctx.state;
    const auto L = widgets::frame(g, ctx.state, "METRICS");
    const int cx = L.cx;

    if (s.metrics.empty()) {
      g.str("no metrics yet", cx, L.cy, theme::kDim, typeface::body(), textdatum_t::middle_center);
      widgets::hints(g, nullptr, nullptr);
      return;
    }
    if (idx_ >= static_cast<int>(s.metrics.size())) idx_ = 0;
    if (confirm_.active()) {
      confirm_.render(g, L);
      return;
    }
    const Metric& m = s.metrics[idx_];
    const int center = L.landscape ? (L.top + 20 + L.bottom) / 2 : L.cy;
    const int trendDy = L.landscape ? 22 : 30;
    widgets::heroValue(g, cx, center, m.label.c_str(), m.value.c_str(),
                       widgets::heroFont(g, m.value.c_str(), L.w - 16));
    widgets::trend(g, cx, center + trendDy, m.trend.empty() ? ' ' : m.trend[0]);
    widgets::dots(g, cx, L.bottom - 6, static_cast<int>(s.metrics.size()), idx_);
    widgets::hints(g, "NEXT", "CLEAR");
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (confirm_.active()) return Transition::none();
    const int n = static_cast<int>(ctx.state.metrics.size());
    if (n <= 1) return Transition::none();
    if (!primed_) {
      primed_ = true;
      lastAdvance_ = nowMs;
      return Transition::none();
    }
    const uint32_t period = (ctx.state.carousel_secs ? ctx.state.carousel_secs : 5) * 1000u;
    if (nowMs - lastAdvance_ < period) return Transition::none();
    lastAdvance_ = nowMs;
    idx_ = (idx_ + 1) % n;
    return Transition::redraw();
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    auto& metrics = ctx.state.metrics;
    const int n = static_cast<int>(metrics.size());
    if (n == 0) return Transition::none();
    if (idx_ >= n) idx_ = 0;

    if (confirm_.active()) {
      if (intent == Intent::Select) {
        ctx.state.removeMetric(metrics[idx_].key);
        confirm_.cancel();
        idx_ = 0;
        primed_ = false;
        return Transition::redraw();
      }
      if (intent == Intent::Next) {
        confirm_.cancel();
        return Transition::redraw();
      }
      return Transition::none();
    }

    if (intent == Intent::Next) {
      confirm_.arm("CLEAR METRIC", metrics[idx_].label);
      return Transition::redraw();
    }
    if (intent == Intent::Select) {
      idx_ = (idx_ + 1) % n;
      lastAdvance_ = 0;
      primed_ = false;
      return Transition::redraw();
    }
    return Transition::none();
  }

 private:
  int idx_ = 0;
  bool primed_ = false;
  uint32_t lastAdvance_ = 0;
  widgets::ConfirmFlow confirm_;
};

}  // namespace

TAMA_SCREEN_FACTORY(metrics, MetricsScreen)

}  // namespace tama::screens
