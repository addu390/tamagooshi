#include "brand.gen.h"
#if TAMA_APP_JIGGLER

#include <cstdio>

#include "apps.h"
#include "hidsession.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr uint32_t kNudgeGapMs = 40000;
constexpr int8_t kNudgePx = 2;

class JigglerScreen : public AppScreen {
 public:
  const char* id() const override { return "app.jiggler"; }

  void onEnter(ShellContext& ctx) override {
    hid_.enter(ctx);
    armed_ = false;
    moves_ = 0;
    nextAt_ = 0;
  }

  void onExit() override { hid_.exit(); }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "JIGGLER");
    hid_.statusPill(g, ctx, L.cx, L.top + 20);

    const int cy = L.cy - 2;
    widgets::pill(g, L.cx, cy - 22, armed_ ? "JIGGLING" : "IDLE", typeface::micro(),
                  armed_ ? theme::kHi : theme::kDim);

    auto& c = g.c();
    const int barW = L.w - 40;
    c.drawRect(20, cy, barW, 6, theme::kDimmer);
    if (armed_ && nextAt_ > now()) {
      const uint32_t left = nextAt_ - now();
      const int fill = static_cast<int>(static_cast<uint64_t>(barW) * (kNudgeGapMs - left) /
                                        kNudgeGapMs);
      c.fillRect(20, cy, fill, 6, theme::kHi);
    }

    char s[16];
    std::snprintf(s, sizeof(s), "MOVES %d", moves_);
    g.str(s, L.cx, cy + 18, theme::kDim, typeface::micro(), textdatum_t::middle_center);

    widgets::hints(g, armed_ ? "STOP" : "START", nullptr);
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent != Intent::Select) return Transition::none();
    armed_ = !armed_;
    nextAt_ = armed_ ? now() + kNudgeGapMs : 0;
    cue(ctx, ExpressionKind::Blink);
    return Transition::redraw();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    AppScreen::tick(ctx, nowMs);
    if (!armed_) return Transition::none();

    if (nowMs >= nextAt_) {
      if (hid_.live()) {
        hid_.nudge(kNudgePx, 0);
        hid_.nudge(-kNudgePx, 0);
        ++moves_;
      }
      nextAt_ = nowMs + kNudgeGapMs;
    }
    return redraw_.due(nowMs, 500) ? Transition::redraw() : Transition::none();
  }

 private:
  MouseSession hid_;
  AnimClock redraw_;
  bool armed_ = false;
  int moves_ = 0;
  uint32_t nextAt_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(jiggler, JigglerScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_JIGGLER
