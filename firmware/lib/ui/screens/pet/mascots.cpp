#include <string>

#include "mascot.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

constexpr int kRows = 2;

class MascotsScreen : public AppScreen {
 public:
  const char* id() const override { return "mascots"; }
  void onEnter(ShellContext&) override { row_ = 0; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "MASCOT");

    const std::string packUp = widgets::upper(category(ctx));
    const std::string nameUp = widgets::upper(ctx.character ? ctx.character->name() : "-");
    const char* vals[kRows] = {packUp.c_str(), nameUp.c_str()};
    static const char* kLabels[kRows] = {"PACK", "MASCOT"};

    const lgfx::IFont* font = widgets::uiFont(L.landscape);
    const int rowH = L.landscape ? 15 : 22;
    const int rowW = L.landscape ? L.w / 2 - 8 : L.w - 8;
    const int top = L.top + 26;
    for (int i = 0; i < kRows; ++i) {
      widgets::listRow(g, {4, top + i * rowH - 2, rowW, rowH - 2}, {kLabels[i], vals[i]},
                       i == row_, font);
    }

    if (ctx.character) {
      if (L.landscape) {
        ctx.character->draw(g, widgets::anchor(L, widgets::Side::Right), L.cy + 6, 52,
                            MascotState{Expr::Happy}, now());
      } else {
        const int size = 56;
        const int cy = (top + kRows * rowH + L.bottom) / 2 - 10;
        ctx.character->draw(g, L.cx, cy, size, MascotState{Expr::Happy}, now());
        widgets::mascotLabel(g, nameUp.c_str(), L.cx, cy, size);
      }
    }

    widgets::hints(g, "CHANGE", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      row_ = cycleIndex(intent, row_, kRows);
      return Transition::redraw();
    }
    if (intent == Intent::Select) {
      if (row_ == 0) {
        cyclePack(ctx);
      } else {
        cycleMascot(ctx);
      }
      return Transition::redraw();
    }
    return Transition::none();
  }

  uint32_t redrawPeriodMs() const override { return 60; }

 private:
  static const char* category(ShellContext& ctx) {
    if (ctx.character) return ctx.character->category();
    return ctx.characters.categoryAt(0);
  }

  void cyclePack(ShellContext& ctx) {
    const int nc = ctx.characters.categoryCount();
    if (nc == 0) return;

    const int ci = ctx.characters.categoryIndexOf(category(ctx));
    for (int step = 1; step <= nc; ++step) {
      const char* cat = ctx.characters.categoryAt((ci + step) % nc);
      if (!ctx.state.enabled.pack(cat)) continue;
      if (auto* c = ctx.characters.inCategory(cat, 0)) {
        ctx.state.character_id = c->id();
        return;
      }
    }
  }

  void cycleMascot(ShellContext& ctx) {
    const char* cat = category(ctx);
    const int within = ctx.characters.indexInCategory(cat, ctx.state.character_id);
    if (auto* c = ctx.characters.inCategory(cat, within + 1)) ctx.state.character_id = c->id();
  }

  int row_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(mascots, MascotsScreen)

}  // namespace tama::screens
