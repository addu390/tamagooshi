#include "brand.gen.h"
#if TAMA_APP_CLOCK

#include <ctime>
#include <cstdio>
#include <cstdlib>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

void formatOffset(char* out, int size, int minutes) {
  const char sign = minutes < 0 ? '-' : '+';
  const int abs = minutes < 0 ? -minutes : minutes;
  std::snprintf(out, size, "UTC%c%02d:%02d", sign, abs / 60, abs % 60);
}

class ClockScreen : public AppScreen {
 public:
  const char* id() const override { return "app.clock"; }
  void onEnter(ShellContext&) override { blink_ = true; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state);

    const std::time_t local = std::time(nullptr) + ctx.state.tz_offset_min * 60;
    std::tm tm{};
    gmtime_r(&local, &tm);

    char hhmm[6];
    std::snprintf(hhmm, sizeof(hhmm), "%02d%c%02d", tm.tm_hour, blink_ ? ':' : ' ', tm.tm_min);
    const int timeY = L.cy - (L.landscape ? 6 : 14);
    const lgfx::IFont* face = widgets::heroFont(g, "00:00", L.w - 12);
    g.str(hhmm, L.cx, timeY, theme::kFg, face, textdatum_t::middle_center);

    char secs[3];
    std::snprintf(secs, sizeof(secs), "%02d", tm.tm_sec);
    if (L.landscape) {
      const int halfW = g.textWidth(hhmm, face) / 2;
      g.str(secs, L.cx + halfW + 6, timeY + 8, theme::kDim, typeface::body(),
            textdatum_t::middle_left);
    } else {
      g.str(secs, L.cx, timeY + 34, theme::kDim, typeface::body(), textdatum_t::middle_center);
    }

    char date[16];
    std::strftime(date, sizeof(date), "%a %d %b", &tm);
    for (char* p = date; *p; ++p) *p = toupper(static_cast<unsigned char>(*p));

    char tz[12];
    formatOffset(tz, sizeof(tz), ctx.state.tz_offset_min);

    if (L.landscape) {
      g.str(date, L.cx, L.bottom - 16, theme::kDim, typeface::micro(), textdatum_t::bottom_center);
      g.str(tz, L.cx, L.bottom - 4, theme::kDimmer, typeface::micro(), textdatum_t::bottom_center);
    } else {
      g.str(date, L.cx, L.bottom - 30, theme::kDim, typeface::micro(), textdatum_t::bottom_center);
      widgets::pill(g, L.cx, L.bottom - 24, tz, typeface::micro(), theme::kDim);
    }

    widgets::hints(g, "TZ +", "TZ -");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) return adjust(ctx, 30);
    if (intent == Intent::Next) return adjust(ctx, -30);
    return Transition::none();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    if (!anim_.due(nowMs, 500)) return Transition::none();
    blink_ = !blink_;
    return Transition::redraw();
  }

 private:
  Transition adjust(ShellContext& ctx, int deltaMin) {
    int next = ctx.state.tz_offset_min + deltaMin;
    if (next > 14 * 60) next = -12 * 60;
    if (next < -12 * 60) next = 14 * 60;
    ctx.state.tz_offset_min = static_cast<int16_t>(next);
    return Transition::redraw();
  }

  AnimClock anim_;
  bool blink_ = true;
};

}  // namespace

TAMA_SCREEN_FACTORY(clock, ClockScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_CLOCK
