#include "brand.gen.h"
#if TAMA_APP_CLICKER

#include <cstdio>

#include "apps.h"
#include "hidsession.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

class ClickerScreen : public AppScreen {
 public:
  const char* id() const override { return "app.clicker"; }
  uint32_t redrawPeriodMs() const override { return 300; }

  void onEnter(ShellContext& ctx) override {
    hid_.enter(ctx);
    slide_ = 1;
    flashAt_ = 0;
    forward_ = true;
  }

  void onExit() override { hid_.exit(); }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "CLICKER");
    hid_.statusPill(g, ctx, L.cx, L.top + 20);

    const int cy = L.cy + 4;
    const bool flash = now() - flashAt_ < 180;
    arrow(g, L.cx - 34, cy, false, flash && !forward_);
    arrow(g, L.cx + 34, cy, true, flash && forward_);

    char s[16];
    std::snprintf(s, sizeof(s), "SLIDE %d", slide_);
    g.str(s, L.cx, L.bottom - 8, theme::kDim, typeface::micro(), textdatum_t::middle_center);

    widgets::hints(g, "NEXT", "PREV");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) {
      send(ctx, KeyboardKey::PageDown, true);
      ++slide_;
      return Transition::redraw();
    }
    if (intent == Intent::Next || intent == Intent::Prev) {
      send(ctx, KeyboardKey::PageUp, false);
      if (slide_ > 1) --slide_;
      return Transition::redraw();
    }
    return Transition::none();
  }

 private:
  void send(ShellContext& ctx, KeyboardKey key, bool forward) {
    hid_.tap(key);
    forward_ = forward;
    flashAt_ = now();
    cue(ctx, ExpressionKind::Blink);
  }

  void arrow(Gfx& g, int cx, int cy, bool right, bool active) {
    auto& c = g.c();
    const int s = 16;
    const int dir = right ? 1 : -1;
    const uint16_t col = active ? theme::kHi : theme::kDim;
    c.fillTriangle(cx + dir * s, cy, cx - dir * s / 2, cy - s, cx - dir * s / 2, cy + s, col);
  }

  KeyboardSession hid_;
  int slide_ = 1;
  bool forward_ = true;
  uint32_t flashAt_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(clicker, ClickerScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_CLICKER
