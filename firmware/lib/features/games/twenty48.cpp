#include "brand.gen.h"
#if TAMA_GAME_TWENTY48

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

class Twenty48Screen : public ArcadeGameScreen {
 public:
  Twenty48Screen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x20482048u); }
  const char* id() const override { return "game.twenty48"; }

 protected:
  const char* title() const override { return "2048"; }
  const char* readyHint() const override { return "A AIM / B SLIDE"; }
  const char* runHint() const override { return "AIM"; }
  const char* hintB() const override { return st_ == St::Run ? "SLIDE" : nullptr; }
  const char* deadTitle() const override { return won_ ? "YOU WIN" : "NO MOVES"; }
  int bannerCenterY() const override { return 46; }
  const lgfx::IFont* bannerHeadFont() const override { return typeface::body(); }

  void renderWorld(Gfx& g, ShellContext&) override {
    auto& c = g.c();
    const int cell = cellSize();
    const int grid = cell * kN;
    const int ox = (w_ - grid) / 2;
    const int oy = gridTop();

    c.drawRect(ox - 2, oy - 2, grid + 4, grid + 4, theme::kDimmer);

    for (int r = 0; r < kN; ++r) {
      for (int c0 = 0; c0 < kN; ++c0) {
        const int v = grid_[r][c0];
        const int x = ox + c0 * cell;
        const int y = oy + r * cell;
        c.drawRect(x, y, cell, cell, theme::kDimmer);
        if (v == 0) continue;
        c.fillRect(x + 1, y + 1, cell - 2, cell - 2, tileFill(v));
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%d", v);
        const lgfx::IFont* font =
            v >= 1024 ? typeface::micro() : (v >= 128 ? typeface::body() : typeface::title());
        g.str(buf, x + cell / 2, y + cell / 2, tileInk(v), font, textdatum_t::middle_center);
      }
    }

    if (st_ == St::Run) drawAim(g, ox + grid / 2, oy + grid + 10);
  }

  Transition onAction(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Select) {
      aim_ = (aim_ + 1) % 4;
      return Transition::redraw();
    }
    if (intent != Intent::Next) return Transition::none();

    int gained = 0;
    if (!slide(aim_, gained)) return Transition::redraw();

    score_ += gained;
    if (gained > 0) cue(ctx, ExpressionKind::Chirp);
    spawn();
    if (!won_ && has2048()) {
      won_ = true;
      cue(ctx, ExpressionKind::Celebrate);
    }
    if (!anyMove()) die();
    return Transition::redraw();
  }

  void onReset() override {
    std::memset(grid_, 0, sizeof(grid_));
    aim_ = 0;
    won_ = false;
    spawn();
    spawn();
  }

  void step(ShellContext&) override {}

 private:
  static constexpr int kN = 4;

  int gridTop() const { return (st_ == St::Ready || st_ == St::Dead) ? 68 : 26; }

  int cellSize() const {
    const int bottom = st_ == St::Run ? 48 : 28;
    return std::min((w_ - 16) / kN, (h_ - gridTop() - bottom) / kN);
  }

  uint16_t tileFill(int v) const {
    if (v >= 2048) return theme::kHi;
    if (v >= 512) return theme::kWarn;
    if (v >= 64) return theme::kFg;
    if (v >= 8) return theme::kDim;
    return theme::kDimmer;
  }

  uint16_t tileInk(int v) const { return v >= 64 ? theme::kBg : theme::kFg; }

  void drawAim(Gfx& g, int cx, int y) const {
    static const char* kArrows[] = {"^", ">", "v", "<"};
    g.str(kArrows[aim_], cx, y, theme::kHi, typeface::title(), textdatum_t::top_center);
  }

  void spawn() {
    int empties[kN * kN];
    int n = 0;
    for (int r = 0; r < kN; ++r) {
      for (int c = 0; c < kN; ++c) {
        if (grid_[r][c] == 0) empties[n++] = r * kN + c;
      }
    }
    if (n == 0) return;
    const int pick = empties[rng_.next() % static_cast<uint32_t>(n)];
    grid_[pick / kN][pick % kN] = (rng_.next() % 10u == 0) ? 4 : 2;
  }

  bool has2048() const {
    for (int r = 0; r < kN; ++r) {
      for (int c = 0; c < kN; ++c) {
        if (grid_[r][c] >= 2048) return true;
      }
    }
    return false;
  }

  bool anyMove() const {
    for (int r = 0; r < kN; ++r) {
      for (int c = 0; c < kN; ++c) {
        const int v = grid_[r][c];
        if (v == 0) return true;
        if (c + 1 < kN && grid_[r][c + 1] == v) return true;
        if (r + 1 < kN && grid_[r + 1][c] == v) return true;
      }
    }
    return false;
  }

  static void pack(int cells[kN], int& gained, bool& moved) {
    int packed[kN] = {};
    int n = 0;
    for (int i = 0; i < kN; ++i) {
      if (cells[i] != 0) packed[n++] = cells[i];
    }

    int out[kN] = {};
    int at = 0;
    for (int i = 0; i < n;) {
      if (i + 1 < n && packed[i] == packed[i + 1]) {
        out[at] = packed[i] * 2;
        gained += out[at];
        ++at;
        i += 2;
      } else {
        out[at++] = packed[i++];
      }
    }

    for (int i = 0; i < kN; ++i) {
      if (cells[i] != out[i]) moved = true;
      cells[i] = out[i];
    }
  }

  bool slide(int dir, int& gained) {
    gained = 0;
    bool moved = false;
    int next[kN][kN];
    std::memcpy(next, grid_, sizeof(grid_));

    if (dir == 0 || dir == 2) {
      for (int c = 0; c < kN; ++c) {
        int col[kN];
        for (int r = 0; r < kN; ++r) col[r] = next[dir == 0 ? r : kN - 1 - r][c];
        pack(col, gained, moved);
        for (int r = 0; r < kN; ++r) next[dir == 0 ? r : kN - 1 - r][c] = col[r];
      }
    } else {
      for (int r = 0; r < kN; ++r) {
        int row[kN];
        for (int c = 0; c < kN; ++c) row[c] = next[r][dir == 3 ? c : kN - 1 - c];
        pack(row, gained, moved);
        for (int c = 0; c < kN; ++c) next[r][dir == 3 ? c : kN - 1 - c] = row[c];
      }
    }

    if (!moved) return false;
    std::memcpy(grid_, next, sizeof(grid_));
    return true;
  }

  int grid_[kN][kN] = {};
  int aim_ = 0;
  bool won_ = false;
};

}  // namespace

TAMA_SCREEN_FACTORY(twenty48, Twenty48Screen)

}  // namespace tama::games

#endif  // TAMA_GAME_TWENTY48
