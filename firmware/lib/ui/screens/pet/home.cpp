#include "mascot.h"
#include "pet.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

constexpr uint32_t kDoubleTapMs = 350;

class HomeScreen : public AppScreen {
 public:
  const char* id() const override { return "home"; }

  void onEnter(ShellContext&) override { tapPending_ = false; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto& s = ctx.state;
    const auto L = widgets::frame(g, s, nullptr);
    if (L.landscape) {
      renderLandscape(g, ctx, L);
    } else {
      renderPortrait(g, ctx, L);
    }
    widgets::hints(g, "PLAY", "MENU");
  }

  void renderPortrait(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const Metric* star = ctx.state.starMetric();
    const int mascotY = star ? L.top + 62 : L.cy;
    drawPet(g, ctx, L.cx, mascotY, 88, 6);
    if (star) {
      const char* value = star->value.c_str();
      widgets::heroValue(g, L.cx, L.bottom - 20, star->label.c_str(), value,
                         metricFont(g, value, L.w - 12), 3);
    }
  }

  void renderLandscape(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const Metric* star = ctx.state.starMetric();
    const int mascotX = star ? widgets::anchor(L, widgets::Side::Left) + 4 : L.cx;
    drawPet(g, ctx, mascotX, L.cy - 6, 66, 8);
    if (star) {
      const char* value = star->value.c_str();
      widgets::heroValue(g, L.w * 5 / 8 + 8, L.cy + 6, star->label.c_str(), value,
                         metricFont(g, value, L.w / 2 - 12));
    }
  }

  static const lgfx::IFont* metricFont(Gfx& g, const char* value, int maxWidth) {
    return widgets::fitFont(g, value, maxWidth, typeface::title(), typeface::body());
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Next) return Transition::push("menu");
    if (intent != Intent::Select) return Transition::none();
    if (tapPending_) {
      tapPending_ = false;
      ctx.state.pet.act(treat_, now());
      treat_ = treat_ == PetAction::Feed ? PetAction::Love : PetAction::Feed;
    } else {
      tapPending_ = true;
      tapAt_ = now();
    }
    return Transition::redraw();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    const Transition t = AppScreen::tick(ctx, nowMs);
    if (tapPending_ && nowMs - tapAt_ >= kDoubleTapMs) {
      tapPending_ = false;
      ctx.state.pet.act(PetAction::Play, nowMs);
      return Transition::redraw();
    }
    return t;
  }

  uint32_t redrawPeriodMs() const override { return 45; }

 private:
  void drawPet(Gfx& g, ShellContext& ctx, int x, int y, int size, int wander) const {
    widgets::pet(g, ctx.character, ctx.state.pet, ctx.mascot, x, y, size, wander, now());
  }

  bool tapPending_ = false;
  PetAction treat_ = PetAction::Feed;
  uint32_t tapAt_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(home, HomeScreen)

}  // namespace tama::screens
