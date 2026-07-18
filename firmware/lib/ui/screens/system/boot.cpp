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
    const int size = 60;
    const int mascotY = g.h() / 2;

    widgets::brandLockup(g, s.branding, cx, 32, g.w() - 20, 26);
    if (!s.branding.tagline.empty()) {
      widgets::wrapText(g, s.branding.tagline.c_str(), cx, 48, g.w() - 12, typeface::micro(),
                        theme::kDim, 10);
    }
    if (ctx.character) ctx.character->draw(g, cx, mascotY, size, MascotState{Expr::Sleepy}, now_);
    widgets::mascotLabel(g, s.branding.mascot_name.c_str(), cx, mascotY, size);
    g.str(s.connected ? "ready" : "waiting for hub", cx, widgets::mascotNameY(mascotY, size) + 16,
          theme::kDim, typeface::body(), textdatum_t::top_center);
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

AppScreen& boot() {
  static BootScreen instance;
  return instance;
}

}  // namespace tama::screens
