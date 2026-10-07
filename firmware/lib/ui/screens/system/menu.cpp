#include <cstring>

#include "apps/apps.h"
#include "brand.gen.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

enum class Icon { Bars, Play, Sliders, Grid, Mic };

struct MenuApp {
  const char* label;
  const char* screen;
  Icon icon;
};

struct Shortcut {
  const char* label;
  const char* app;
  Icon icon;
};

constexpr MenuApp kApps[] = {
    {"METRICS", "metrics", Icon::Bars},
    {"GAMES", "play", Icon::Play},
    {"SETTINGS", "settings", Icon::Sliders},
    {"APPS", "apps", Icon::Grid},
};
constexpr Shortcut kShortcuts[] = {
    {"ASK", "talk", Icon::Mic},
};
constexpr int kMax = sizeof(kApps) / sizeof(kApps[0]) + sizeof(kShortcuts) / sizeof(kShortcuts[0]);

// The screen of an enabled, usable app, or nullptr.
const char* appScreen(const char* id, const ShellContext& ctx) {
  if (!ctx.state.enabled.app(id)) return nullptr;
  const FeatureInfo* items = apps::list();
  for (int i = 0; i < apps::count(); ++i) {
    if (std::strcmp(items[i].id, id) != 0) continue;
    return locked(items[i], ctx.caps, ctx.hidProfile) ? nullptr : items[i].screen;
  }
  return nullptr;
}

int collect(const ShellContext& ctx, MenuApp* out) {
  int n = 0;
  for (const Shortcut& s : kShortcuts) {
    if (const char* screen = appScreen(s.app, ctx)) out[n++] = {s.label, screen, s.icon};
  }
  for (const MenuApp& app : kApps) out[n++] = app;
  return n;
}

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
  void onEnter(ShellContext& ctx) override {
    sel_ = 0;
    count_ = collect(ctx, apps_);
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "MENU");
    const auto gr = widgets::grid(L, count_, 2, 3, L.top + 20, 8, 8, 46);

    for (int i = 0; i < count_; ++i) {
      const auto r = gr.cell(i, count_);
      const int cx = r.x + r.w / 2;
      const uint16_t fg = widgets::selectionBox(g, r, i == sel_, widgets::SelectStyle{});
      drawIcon(g.c(), apps_[i].icon, cx, r.y + r.h / 2 - 6, fg);
      g.str(apps_[i].label, cx, r.y + r.h - 9, fg, typeface::micro(), textdatum_t::middle_center);
    }
    widgets::hints(g, "OPEN", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      sel_ = cycleIndex(intent, sel_, count_);
      return Transition::redraw();
    }
    if (intent == Intent::Select) return Transition::push(apps_[sel_].screen);
    return Transition::none();
  }

 private:
  MenuApp apps_[kMax];
  int count_ = 0;
  int sel_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(menu, MenuScreen)

}  // namespace tama::screens
