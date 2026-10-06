#include "mascot.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

class BootScreen : public AppScreen {
 public:
  const char* id() const override { return "boot"; }

  void onEnter(ShellContext&) override { start_ = 0; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto& s = ctx.state;
    const int cx = g.w() / 2;
    const bool landscape = g.w() > g.h();
    const int size = landscape ? 44 : 66;
    const int mascotX = landscape ? cx - 36 : cx;
    const int mascotY = landscape ? g.h() / 2 + 8 : g.h() / 2 - 4;
    const char* status = s.connected ? "ready" : "waiting for hub";

    widgets::brandLockup(g, s.branding, cx, 32, g.w() - 20, 26);
    if (ctx.character) {
      ctx.character->draw(g, mascotX, mascotY, size, MascotState{Expr::Sleepy}, now_);
    }
    if (landscape) {
      g.str(status, cx + 4, mascotY, theme::kDim, typeface::body(), textdatum_t::middle_left);
    } else {
      g.str(status, cx, widgets::mascotNameY(mascotY, size), theme::kDim, typeface::body(),
            textdatum_t::top_center);
    }
    widgets::hints(g, "START", nullptr);
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    return intent == Intent::Select ? Transition::replace("home") : Transition::none();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    if (start_ == 0) start_ = nowMs;
    now_ = nowMs;
    if (nowMs - start_ > 2500) return Transition::replace("home");
    return anim_.due(nowMs, 60) ? Transition::redraw() : Transition::none();
  }

 private:
  uint32_t now_ = 0;
  uint32_t start_ = 0;
  AnimClock anim_;
};

}  // namespace

TAMA_SCREEN_FACTORY(boot, BootScreen)

}  // namespace tama::screens
