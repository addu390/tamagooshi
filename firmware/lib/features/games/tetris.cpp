#include "brand.gen.h"
#if TAMA_GAME_TETRIS

#include <cstring>

#include "arcade.h"
#include "games.h"
#include "input.h"

namespace tama::games {

namespace {

constexpr int8_t kShapes[7][4][4][2] = {
    {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}},
     {{1, -1}, {1, 0}, {1, 1}, {1, 2}},
     {{-1, 1}, {0, 1}, {1, 1}, {2, 1}},
     {{0, -1}, {0, 0}, {0, 1}, {0, 2}}},

    {{{0, 0}, {1, 0}, {0, 1}, {1, 1}},
     {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
     {{0, 0}, {1, 0}, {0, 1}, {1, 1}},
     {{0, 0}, {1, 0}, {0, 1}, {1, 1}}},

    {{{-1, 0}, {0, 0}, {1, 0}, {0, 1}},
     {{0, -1}, {0, 0}, {0, 1}, {1, 0}},
     {{-1, 0}, {0, 0}, {1, 0}, {0, -1}},
     {{0, -1}, {0, 0}, {0, 1}, {-1, 0}}},

    {{{0, 0}, {1, 0}, {-1, 1}, {0, 1}},
     {{0, -1}, {0, 0}, {1, 0}, {1, 1}},
     {{0, 0}, {1, 0}, {-1, 1}, {0, 1}},
     {{0, -1}, {0, 0}, {1, 0}, {1, 1}}},

    {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}},
     {{1, -1}, {0, 0}, {1, 0}, {0, 1}},
     {{-1, 0}, {0, 0}, {0, 1}, {1, 1}},
     {{1, -1}, {0, 0}, {1, 0}, {0, 1}}},

    {{{-1, 0}, {0, 0}, {1, 0}, {-1, 1}},
     {{0, -1}, {0, 0}, {0, 1}, {1, 1}},
     {{-1, 0}, {0, 0}, {1, 0}, {1, -1}},
     {{0, -1}, {-1, -1}, {0, 0}, {0, 1}}},

    {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}},
     {{0, -1}, {0, 0}, {0, 1}, {1, -1}},
     {{-1, 0}, {0, 0}, {1, 0}, {-1, -1}},
     {{0, -1}, {0, 0}, {0, 1}, {-1, 1}}},
};

class TetrisScreen : public ArcadeGameScreen {
 public:
  TetrisScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x7e7715u); }
  const char* id() const override { return "game.tetris"; }

 protected:
  const char* title() const override { return "TETRIS"; }
  const char* readyHint() const override { return "A/B MOVE · A ROT"; }
  const char* runHint() const override { return "LEFT"; }
  const char* hintB() const override { return st_ == St::Run ? "RIGHT" : nullptr; }
  const char* deadTitle() const override { return "TOPPED OUT"; }
  int bannerCenterY() const override { return 40; }
  const lgfx::IFont* bannerHeadFont() const override { return typeface::body(); }

  void renderWorld(Gfx& g, ShellContext&) override {
    auto& c = g.c();
    const int ox = originX();
    const int oy = kTop;

    c.drawRect(ox - 1, oy - 1, kCols * cell() + 2, kRows * cell() + 2, theme::kDimmer);

    for (int r = 0; r < kRows; ++r) {
      for (int col = 0; col < kCols; ++col) {
        if (!board_[r][col]) continue;
        fillCell(c, ox, oy, col, r, theme::kFg);
      }
    }

    if (st_ == St::Run || st_ == St::Ready) {
      forEachCell(kind_, rot_, px_, py_, [&](int col, int row) {
        if (row < 0) return;
        fillCell(c, ox, oy, col, row, theme::kHi);
      });
    }

    drawNext(c);
  }

  Transition onAction(Intent intent, ShellContext& ctx) override {
    if (intent != Intent::Select) return Transition::none();
    tryRotate(ctx);
    return Transition::redraw();
  }

  void onReset() override {
    std::memset(board_, 0, sizeof(board_));
    bagCount_ = 0;
    fallAcc_ = 0;
    moveAcc_ = 0;
    next_ = deal();
    spawn();
  }

  void step(ShellContext& ctx) override {
    moveAcc_ += kStepMs;

    int dir = 0;
    if (ctx.buttons.held(0)) dir -= 1;
    if (ctx.buttons.held(1)) dir += 1;

    if (dir != 0 && moveAcc_ >= kDasMs) {
      moveAcc_ = 0;
      if (fits(kind_, rot_, px_ + dir, py_)) px_ += dir;
    }

    fallAcc_ += kStepMs;
    const uint32_t interval = static_cast<uint32_t>(kFallRamp.at(elapsedSec()));
    if (fallAcc_ < interval) return;
    fallAcc_ = 0;

    if (fits(kind_, rot_, px_, py_ + 1)) {
      ++py_;
    } else {
      lockPiece(ctx);
    }
  }

 private:
  static constexpr int kCols = 10;
  static constexpr int kRows = 18;
  static constexpr int kTop = 22;
  static constexpr int kBottom = 18;
  static constexpr int kDasMs = 90;
  static constexpr DifficultyRamp kFallRamp{520.0f, -8.0f, 120.0f, 4.0f};

  int cell() const {
    const int byW = (w_ - 8) / kCols;
    const int byH = (h_ - kTop - kBottom) / kRows;
    return byW < byH ? byW : byH;
  }

  int originX() const { return (w_ - kCols * cell()) / 2; }

  void fillCell(M5Canvas& c, int ox, int oy, int col, int row, uint16_t color) const {
    const int s = cell();
    c.fillRect(ox + col * s + 1, oy + row * s + 1, s - 2, s - 2, color);
  }

  void drawNext(M5Canvas& c) const {
    const int s = 5;
    const int ox = w_ - 4 - 4 * s;
    const int oy = kTop;

    c.drawRect(ox - 2, oy - 2, 4 * s + 4, 4 * s + 4, theme::kDimmer);

    forEachCell(next_, 0, 1, 1, [&](int col, int row) {
      c.fillRect(ox + col * s + 1, oy + row * s + 1, s - 2, s - 2, theme::kDim);
    });
  }

  template <class Fn>
  static void forEachCell(int kind, int rot, int px, int py, Fn&& fn) {
    for (int i = 0; i < 4; ++i) {
      const int col = px + kShapes[kind][rot][i][0];
      const int row = py + kShapes[kind][rot][i][1];
      fn(col, row);
    }
  }

  bool fits(int kind, int rot, int px, int py) const {
    bool ok = true;

    forEachCell(kind, rot, px, py, [&](int col, int row) {
      if (col < 0 || col >= kCols || row >= kRows) {
        ok = false;
        return;
      }
      if (row >= 0 && board_[row][col]) ok = false;
    });

    return ok;
  }

  void tryRotate(ShellContext& ctx) {
    const int next = (rot_ + 1) & 3;
    static constexpr int kKicks[] = {0, -1, 1, -2, 2};

    for (int kick : kKicks) {
      if (fits(kind_, next, px_ + kick, py_)) {
        rot_ = next;
        px_ += kick;
        cue(ctx, ExpressionKind::Tick);
        return;
      }
    }
  }

  void lockPiece(ShellContext& ctx) {
    forEachCell(kind_, rot_, px_, py_, [&](int col, int row) {
      if (row < 0 || row >= kRows || col < 0 || col >= kCols) return;
      board_[row][col] = true;
    });

    int cleared = 0;

    for (int r = kRows - 1; r >= 0; --r) {
      bool full = true;
      for (int col = 0; col < kCols; ++col) {
        if (!board_[r][col]) full = false;
      }
      if (!full) continue;

      ++cleared;
      for (int rr = r; rr > 0; --rr) {
        std::memcpy(board_[rr], board_[rr - 1], sizeof(board_[0]));
      }
      std::memset(board_[0], 0, sizeof(board_[0]));
      ++r;
    }

    if (cleared > 0) {
      static constexpr int kPts[] = {0, 1, 3, 5, 8};
      score_ += kPts[cleared];
      cue(ctx, ExpressionKind::Chirp);
    }

    spawn();
  }

  int deal() {
    if (bagCount_ == 0) {
      for (int i = 0; i < 7; ++i) bag_[i] = static_cast<int8_t>(i);

      for (int i = 6; i > 0; --i) {
        const int j = static_cast<int>(rng_.next() % static_cast<uint32_t>(i + 1));
        const int8_t tmp = bag_[i];
        bag_[i] = bag_[j];
        bag_[j] = tmp;
      }

      bagCount_ = 7;
    }

    return bag_[--bagCount_];
  }

  void spawn() {
    kind_ = next_;
    next_ = deal();
    rot_ = 0;
    px_ = kCols / 2 - 1;
    py_ = 0;
    fallAcc_ = 0;

    if (!fits(kind_, rot_, px_, py_)) die();
  }

  bool board_[kRows][kCols] = {};
  int kind_ = 0;
  int next_ = 0;
  int rot_ = 0;
  int px_ = 0;
  int py_ = 0;
  int8_t bag_[7] = {};
  int bagCount_ = 0;
  uint32_t fallAcc_ = 0;
  uint32_t moveAcc_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(tetris, TetrisScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_TETRIS
