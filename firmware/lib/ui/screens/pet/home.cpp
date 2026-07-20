#include "mascot.h"
#include "pet.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

class HomeScreen : public AppScreen {
 public:
  const char* id() const override { return "home"; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto& s = ctx.state;
    const auto L = widgets::frame(g, s, nullptr, false);
    if (L.landscape) {
      renderLandscape(g, ctx, L);
    } else {
      renderPortrait(g, ctx, L);
    }
  }

  void renderPortrait(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const auto& s = ctx.state;
    const int cx = L.cx;
    if (s.connected) {
      widgets::brandLockup(g, s.branding, cx, L.top + 18, L.w - 20, 26);
      if (!s.branding.tagline.empty()) {
        widgets::wrapText(g, s.branding.tagline.c_str(), cx, L.top + 34, L.w - 12,
                          typeface::micro(), theme::kDim, 10);
      }
      const int size = 56;
      const int mascotY = L.cy - 14;
      if (ctx.character) {
        MascotState m = ctx.mascot;
        m.wanderPx = 10;
        ctx.character->draw(g, cx, mascotY, size, m, now());
      }
      widgets::mascotLabel(g, s.branding.mascot_name.c_str(), cx, mascotY, size);
      const Metric* star = s.starMetric();
      if (star) {
        widgets::heroValue(g, cx, L.bottom - 28, star->label.c_str(), star->value.c_str(),
                           widgets::fitFont(g, star->value.c_str(), L.w - 16, typeface::title(),
                                            typeface::body()));
      }
      widgets::hints(g, "MENU", "NOOK");
    } else {
      if (ctx.character) {
        ctx.character->draw(g, cx, L.cy - 20, 56, MascotState{exprFromPet(ctx.pet), 12}, now());
      }
      widgets::statBar(g, L.bottom - 48, "NRG", ctx.pet.energy);
      widgets::statBar(g, L.bottom - 22, "CARE", ctx.pet.care);
      widgets::hints(g, "FEED", "PLAY");
    }
  }

  void renderLandscape(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const auto& s = ctx.state;
    const int mascotX = widgets::anchor(L, widgets::Side::Left);
    const int rcx = L.w * 5 / 8;
    if (s.connected) {
      widgets::brandLockup(g, s.branding, rcx, L.top + 14, L.w / 2 - 12);
      const int size = 52;
      const int mascotY = L.cy;
      if (ctx.character) {
        MascotState m = ctx.mascot;
        m.wanderPx = 8;
        ctx.character->draw(g, mascotX, mascotY, size, m, now());
      }
      widgets::mascotLabel(g, s.branding.mascot_name.c_str(), mascotX, mascotY, size);
      const Metric* star = s.starMetric();
      if (star) {
        widgets::heroValue(g, rcx, L.cy + 18, star->label.c_str(), star->value.c_str(),
                           typeface::title());
      }
      widgets::hints(g, "MENU", "NOOK");
    } else {
      if (ctx.character) {
        ctx.character->draw(g, mascotX, L.cy, 52, MascotState{exprFromPet(ctx.pet), 10}, now());
      }
      const int bx = L.w / 2 + 4;
      const int bw = L.w - bx - 8;
      widgets::statBarAt(g, bx, L.cy - 20, bw, "NRG", ctx.pet.energy);
      widgets::statBarAt(g, bx, L.cy + 10, bw, "CARE", ctx.pet.care);
      widgets::hints(g, "FEED", "PLAY");
    }
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (ctx.state.connected) {
      if (intent == Intent::Select) return Transition::push("menu");
      if (intent == Intent::Next) return Transition::push("nook");
      return Transition::none();
    }
    if (intent == Intent::Select) {
      petSim_.quickFeed(ctx.pet);
      return Transition::redraw();
    }
    if (intent == Intent::Next) return Transition::push("play");
    return Transition::none();
  }

  uint32_t redrawPeriodMs() const override { return 45; }

 private:
  Care petSim_;
};

}  // namespace

TAMA_SCREEN_FACTORY(home, HomeScreen)

}  // namespace tama::screens
