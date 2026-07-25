#include "brand.gen.h"
#if TAMA_APP_MEDIA

#include "apps.h"
#include "hidsession.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

struct Action {
  const char* glyph;
  const char* label;
  MediaKey key;
};

constexpr Action kActions[] = {
    {"<<", "PREV", MediaKey::Prev},        {">", "PLAY", MediaKey::PlayPause},
    {">>", "NEXT", MediaKey::Next},        {"-", "VOL-", MediaKey::VolumeDown},
    {"+", "VOL+", MediaKey::VolumeUp},
};
constexpr int kActionCount = sizeof(kActions) / sizeof(kActions[0]);

class MediaScreen : public AppScreen {
 public:
  const char* id() const override { return "app.media"; }
  uint32_t redrawPeriodMs() const override { return 300; }

  void onEnter(ShellContext& ctx) override {
    hid_.enter(ctx);
    sel_ = 1;
    sentAt_ = 0;
  }

  void onExit() override { hid_.exit(); }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "MEDIA");
    hid_.statusPill(g, ctx, L.cx, L.top + 20);

    const auto grid = widgets::grid(L, kActionCount, 3, 5, L.top + 44, 6, 10, 44);
    for (int i = 0; i < kActionCount; ++i) {
      const auto r = grid.cell(i, kActionCount);
      const bool flash = i == sel_ && now() - sentAt_ < 180;
      const uint16_t content = widgets::selectionBox(g, r, i == sel_ || flash);
      g.str(kActions[i].glyph, r.x + r.w / 2, r.y + r.h / 2, content, typeface::body(),
            textdatum_t::middle_center);
    }

    g.str(kActions[sel_].label, L.cx, L.bottom - 8, theme::kDim, typeface::micro(),
          textdatum_t::middle_center);
    widgets::hints(g, "SEND", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      sel_ = cycleIndex(intent, sel_, kActionCount);
      return Transition::redraw();
    }
    if (intent == Intent::Select) {
      hid_.tap(kActions[sel_].key);
      sentAt_ = now();
      cue(ctx, ExpressionKind::Blink);
      return Transition::redraw();
    }
    return Transition::none();
  }

 private:
  MediaSession hid_;
  int sel_ = 1;
  uint32_t sentAt_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(media, MediaScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_MEDIA
