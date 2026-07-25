#include "brand.gen.h"
#if TAMA_APP_VOLUME

#include "apps.h"
#include "hidsession.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kSteps = 16;

class VolumeScreen : public AppScreen {
 public:
  const char* id() const override { return "app.volume"; }
  uint32_t redrawPeriodMs() const override { return 300; }

  void onEnter(ShellContext& ctx) override {
    hid_.enter(ctx);
    level_ = kSteps / 2;
    flashAt_ = 0;
  }

  void onExit() override { hid_.exit(); }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "VOLUME");
    hid_.statusPill(g, ctx, L.cx, L.top + 20);

    auto& c = g.c();
    const int cy = L.cy + 8;
    const int barW = L.w - 56;
    const int stepW = barW / kSteps;
    const bool flash = now() - flashAt_ < 180;

    for (int i = 0; i < kSteps; ++i) {
      const int x = L.cx - barW / 2 + i * stepW;
      const int hgt = 6 + i * 14 / kSteps;
      const uint16_t col = i < level_ ? (flash ? theme::kHi : theme::kFg) : theme::kDimmer;
      c.fillRect(x, cy - hgt, stepW - 2, hgt, col);
    }

    g.str("-", L.cx - barW / 2 - 12, cy - 8, theme::kDim, typeface::body(),
          textdatum_t::middle_center);
    g.str("+", L.cx + barW / 2 + 12, cy - 8, theme::kDim, typeface::body(),
          textdatum_t::middle_center);

    widgets::hints(g, "VOL +", "VOL -");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) return bump(ctx, MediaKey::VolumeUp, 1);
    if (intent == Intent::Next || intent == Intent::Prev)
      return bump(ctx, MediaKey::VolumeDown, -1);
    return Transition::none();
  }

 private:
  Transition bump(ShellContext& ctx, MediaKey key, int delta) {
    hid_.tap(key);
    level_ += delta;
    if (level_ < 0) level_ = 0;
    if (level_ > kSteps) level_ = kSteps;
    flashAt_ = now();
    cue(ctx, ExpressionKind::Blink);
    return Transition::redraw();
  }

  MediaSession hid_;
  int level_ = kSteps / 2;
  uint32_t flashAt_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(volume, VolumeScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_VOLUME
