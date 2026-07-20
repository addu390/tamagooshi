#include "mascot.h"
#include "pet.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

constexpr int kActs = 3;

widgets::StatStyle statStyle() {
  widgets::StatStyle s;
  s.font = typeface::micro();
  s.rounded = true;
  s.critThreshold = 25;
  s.labelW = 34;
  s.barH = 9;
  return s;
}

class NookScreen : public AppScreen {
 public:
  const char* id() const override { return "nook"; }

  void onEnter(ShellContext&) override {
    sel_ = 0;
    sim_.reset();
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state);

    const bool reacting = sim_.reacting(now_);
    const Expr expr = reacting ? sim_.reactExpr() : exprFromPet(ctx.pet);
    const MascotState ms{expr, reacting ? 0 : 10};
    const widgets::StatStyle stat = statStyle();

    if (L.landscape) {
      const int mascotX = widgets::anchor(L, widgets::Side::Left);
      if (ctx.character) ctx.character->draw(g, mascotX, L.cy, 52, ms, now_);
      drawParticles(g, mascotX, L.cy);

      const int x0 = L.w / 2 + 4;
      const int rw = L.w - x0 - 8;
      widgets::statBarAt(g, x0, L.top + 2, rw, "NRG", ctx.pet.energy, stat);
      widgets::statBarAt(g, x0, L.top + 20, rw, "CARE", ctx.pet.care, stat);
      widgets::statBarAt(g, x0, L.top + 38, rw, "BOND", ctx.pet.bond, stat);
      const int ty = L.top + 54;
      drawTiles(g, x0, ty, rw, L.bottom - ty - 2);
    } else {
      const int mascotY = L.top + 44;
      if (ctx.character) ctx.character->draw(g, L.cx, mascotY, 58, ms, now_);
      drawParticles(g, L.cx, mascotY);

      const int x0 = 8;
      const int rw = L.w - 16;
      widgets::statBarAt(g, x0, L.bottom - 100, rw, "NRG", ctx.pet.energy, stat);
      widgets::statBarAt(g, x0, L.bottom - 82, rw, "CARE", ctx.pet.care, stat);
      widgets::statBarAt(g, x0, L.bottom - 64, rw, "BOND", ctx.pet.bond, stat);
      drawTiles(g, x0, L.bottom - 44, rw, 40);
    }

    widgets::hints(g, "DO", "PICK");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      sel_ = cycleIndex(intent, sel_, kActs);
      return Transition::redraw();
    }
    if (intent == Intent::Select) {
      sim_.doAction(ctx.pet, static_cast<PetAction>(sel_), now_);
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    now_ = nowMs;
    bool dirty = sim_.decay(ctx.pet, nowMs);
    dirty |= sim_.spontaneous(ctx.pet, nowMs);
    if (anim_.due(nowMs, 33)) {
      sim_.stepParticles();
      dirty = true;
    }
    return dirty ? Transition::redraw() : Transition::none();
  }

 private:
  void drawParticles(Gfx& g, int anchorX, int anchorY) {
    for (const auto& p : sim_.particles()) {
      const int x = anchorX + static_cast<int>(p.x);
      const int y = anchorY + static_cast<int>(p.y);
      switch (p.kind) {
        case Fx::Heart: drawHeart(g.c(), x, y, theme::kBlush); break;
        case Fx::Berry: drawBerry(g.c(), x, y); break;
        case Fx::Star: drawStar(g.c(), x, y); break;
      }
    }
  }

  void drawTiles(Gfx& g, int x0, int ty, int totalW, int th) {
    static const char* kLabels[kActs] = {"FEED", "PLAY", "LOVE"};
    const int gap = 8;
    const int tileW = (totalW - gap * (kActs - 1)) / kActs;
    for (int i = 0; i < kActs; ++i) {
      const int x = x0 + i * (tileW + gap);
      const int tcx = x + tileW / 2;
      const uint16_t fg = widgets::selectionBox(g, {x, ty, tileW, th}, i == sel_);
      drawActIcon(g.c(), static_cast<PetAction>(i), tcx, ty + 14, fg);
      g.str(kLabels[i], tcx, ty + th - 9, fg, typeface::micro(), textdatum_t::middle_center);
    }
  }

  static void drawHeart(M5Canvas& c, int x, int y, uint16_t col) {
    c.fillCircle(x - 2, y, 2, col);
    c.fillCircle(x + 2, y, 2, col);
    c.fillTriangle(x - 4, y + 1, x + 4, y + 1, x, y + 6, col);
  }

  static void drawBerry(M5Canvas& c, int x, int y) {
    const uint16_t body = 0xC206;
    const uint16_t leaf = 0x4BC6;
    const uint16_t seed = 0xEF14;
    c.fillCircle(x - 1, y, 2, body);
    c.fillCircle(x + 1, y, 2, body);
    c.fillTriangle(x - 3, y, x + 3, y, x, y + 4, body);
    c.drawFastHLine(x - 2, y - 3, 5, leaf);
    c.drawPixel(x, y - 4, leaf);
    c.drawPixel(x - 1, y, seed);
    c.drawPixel(x + 1, y + 1, seed);
  }

  static void drawStar(M5Canvas& c, int x, int y) {
    const uint16_t col = 0xFEE0;
    c.fillTriangle(x, y - 5, x - 2, y, x + 2, y, col);
    c.fillTriangle(x, y + 5, x - 2, y, x + 2, y, col);
    c.fillTriangle(x - 5, y, x, y - 2, x, y + 2, col);
    c.fillTriangle(x + 5, y, x, y - 2, x, y + 2, col);
  }

  static void drawActIcon(M5Canvas& c, PetAction a, int cx, int cy, uint16_t col) {
    switch (a) {
      case PetAction::Feed:
        c.fillCircle(cx, cy + 1, 7, col);
        c.drawFastVLine(cx, cy - 8, 4, col);
        break;
      case PetAction::Play:
        c.drawCircle(cx, cy, 7, col);
        c.drawLine(cx - 7, cy, cx + 7, cy, col);
        c.drawLine(cx, cy - 7, cx, cy + 7, col);
        break;
      case PetAction::Love:
        c.fillCircle(cx - 3, cy - 1, 3, col);
        c.fillCircle(cx + 3, cy - 1, 3, col);
        c.fillTriangle(cx - 6, cy + 1, cx + 6, cy + 1, cx, cy + 8, col);
        break;
    }
  }

  int sel_ = 0;
  uint32_t now_ = 0;
  AnimClock anim_;
  Care sim_;
};

}  // namespace

TAMA_SCREEN_FACTORY(nook, NookScreen)

}  // namespace tama::screens
