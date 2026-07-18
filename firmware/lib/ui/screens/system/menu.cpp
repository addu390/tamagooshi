#include "brand.gen.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

enum class Icon { Bars, Play, Sliders, Grid, Spark, Mic };

struct MenuApp {
  const char* label;
  const char* screen;
  bool enabled;
  Icon icon;
};

constexpr MenuApp kApps[] = {
    {"METRICS", "metrics", true, Icon::Bars},
    {"PLAY", "play", true, Icon::Play},
    {"SETTINGS", "settings", true, Icon::Sliders},
    {"APPS", "apps", true, Icon::Grid},
#if defined(TAMA_ENABLE_BUDDY)
    {"BUDDY", "buddy", true, Icon::Spark},
    {"ASK", "ask", true, Icon::Mic},
#endif
};
constexpr int kCount = sizeof(kApps) / sizeof(kApps[0]);

void drawIcon(M5Canvas& c, Icon ic, int cx, int cy, uint16_t col) {
  switch (ic) {
    case Icon::Bars:
      c.fillRect(cx - 8, cy + 1, 4, 5, col);
      c.fillRect(cx - 2, cy - 3, 4, 9, col);
      c.fillRect(cx + 4, cy - 7, 4, 13, col);
      break;
    case Icon::Play:
      c.fillTriangle(cx - 5, cy - 7, cx - 5, cy + 7, cx + 8, cy, col);
      break;
    case Icon::Sliders:
      c.drawFastHLine(cx - 9, cy - 4, 18, col);
      c.drawFastHLine(cx - 9, cy + 4, 18, col);
      c.fillCircle(cx - 1, cy - 4, 2, col);
      c.fillCircle(cx + 4, cy + 4, 2, col);
      break;
    case Icon::Grid:
      c.fillRect(cx - 8, cy - 8, 6, 6, col);
      c.fillRect(cx + 2, cy - 8, 6, 6, col);
      c.fillRect(cx - 8, cy + 2, 6, 6, col);
      c.fillRect(cx + 2, cy + 2, 6, 6, col);
      break;
    case Icon::Spark:
      c.drawLine(cx, cy - 8, cx, cy + 8, col);
      c.drawLine(cx - 8, cy, cx + 8, cy, col);
      c.drawLine(cx - 5, cy - 5, cx + 5, cy + 5, col);
      c.drawLine(cx - 5, cy + 5, cx + 5, cy - 5, col);
      break;
    case Icon::Mic:
      c.fillRoundRect(cx - 3, cy - 8, 6, 10, 3, col);
      c.drawFastHLine(cx - 6, cy + 3, 12, col);
      c.drawFastVLine(cx, cy + 4, 4, col);
      c.drawFastHLine(cx - 3, cy + 8, 6, col);
      break;
  }
}

class MenuScreen : public AppScreen {
 public:
  const char* id() const override { return "menu"; }
  void onEnter(ShellContext&) override { sel_ = 0; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "MENU");
    const auto gr = widgets::grid(L, kCount, 2, 3, L.top + 20, 8, 8, 46);

    for (int i = 0; i < kCount; ++i) {
      const auto r = gr.cell(i, kCount);
      const int cx = r.x + r.w / 2;
      const bool on = kApps[i].enabled;

      widgets::SelectStyle style;
      style.fill = on ? theme::kFg : theme::kDim;
      style.outline = on ? theme::kDim : theme::kDimmer;
      style.content = on ? theme::kFg : theme::kDimmer;
      const uint16_t fg = widgets::selectionBox(g, r, i == sel_, style);

      drawIcon(g.c(), kApps[i].icon, cx, r.y + r.h / 2 - 6, fg);
      g.str(kApps[i].label, cx, r.y + r.h - 9, fg, typeface::micro(), textdatum_t::middle_center);
    }
    widgets::hints(g, "OPEN", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      sel_ = cycleIndex(intent, sel_, kCount);
      return Transition::redraw();
    }
    if (intent == Intent::Select && kApps[sel_].enabled) {
      return Transition::push(kApps[sel_].screen);
    }
    return Transition::none();
  }

 private:
  int sel_ = 0;
};

}  // namespace

AppScreen& menu() {
  static MenuScreen instance;
  return instance;
}

}  // namespace tama::screens
