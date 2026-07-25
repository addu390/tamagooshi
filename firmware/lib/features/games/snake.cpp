#include "brand.gen.h"
#if TAMA_GAME_SNAKE

#include <vector>

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

struct Cell {
  int8_t col;
  int8_t row;

  bool operator==(const Cell& o) const { return col == o.col && row == o.row; }
};

class SnakeScreen : public ArcadeGameScreen {
 public:
  SnakeScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x5eedf00du); }
  const char* id() const override { return "game.snake"; }

 protected:
  const char* title() const override { return "SNAKE"; }
  const char* readyHint() const override { return "A LEFT / B RIGHT"; }
  const char* runHint() const override { return "LEFT"; }
  const char* hintB() const override { return st_ == St::Run ? "RIGHT" : nullptr; }

  void renderWorld(Gfx& g, ShellContext&) override {
    auto& c = g.c();
    c.drawRect(originX() - 1, kTop - 1, cols() * kCell + 2, rows() * kCell + 2, theme::kDimmer);

    for (size_t i = 0; i < body_.size(); ++i) {
      const auto [x, y] = px(body_[i]);
      const bool head = i + 1 == body_.size();
      c.fillRect(x + 1, y + 1, kCell - 2, kCell - 2, head ? theme::kHi : theme::kFg);
    }

    const auto [fx, fy] = px(food_);
    c.fillCircle(fx + kCell / 2, fy + kCell / 2, kCell / 2 - 1, theme::kWarn);
  }

  Transition onAction(Intent intent, ShellContext&) override {
    if (intent == Intent::Select) {
      turn_ = 3;
    } else if (intent == Intent::Next) {
      turn_ = 1;
    } else {
      return Transition::none();
    }
    return Transition::redraw();
  }

  void onReset() override {
    body_.clear();
    const int8_t cx = static_cast<int8_t>(cols() / 2);
    const int8_t cy = static_cast<int8_t>(rows() / 2);
    for (int8_t i = 2; i >= 0; --i) body_.push_back({static_cast<int8_t>(cx - i), cy});
    dir_ = 1;
    turn_ = 0;
    grow_ = 0;
    moveAcc_ = 0;
    dropFood();
  }

  void step(ShellContext& ctx) override {
    moveAcc_ += kStepMs;
    const uint32_t interval = static_cast<uint32_t>(kPaceRamp.at(elapsedSec()));
    if (moveAcc_ < interval) return;
    moveAcc_ = 0;

    if (turn_ != 0) {
      dir_ = (dir_ + turn_) % 4;
      turn_ = 0;
    }

    Cell head = body_.back();
    head.col += static_cast<int8_t>(dir_ == 1 ? 1 : (dir_ == 3 ? -1 : 0));
    head.row += static_cast<int8_t>(dir_ == 2 ? 1 : (dir_ == 0 ? -1 : 0));

    if (head.col < 0 || head.col >= cols() || head.row < 0 || head.row >= rows()) return die();
    for (const auto& c : body_)
      if (c == head) return die();

    body_.push_back(head);
    if (head == food_) {
      ++score_;
      grow_ += 2;
      cue(ctx, ExpressionKind::Chirp);
      dropFood();
    }
    if (grow_ > 0) {
      --grow_;
    } else {
      body_.erase(body_.begin());
    }
  }

 private:
  int cols() const { return (w_ - 4) / kCell; }
  int rows() const { return (h_ - kTop - kBottom) / kCell; }
  int originX() const { return (w_ - cols() * kCell) / 2; }

  std::pair<int, int> px(const Cell& c) const {
    return {originX() + c.col * kCell, kTop + c.row * kCell};
  }

  void dropFood() {
    for (int tries = 0; tries < 64; ++tries) {
      const Cell c{static_cast<int8_t>(rng_.next() % static_cast<uint32_t>(cols())),
                   static_cast<int8_t>(rng_.next() % static_cast<uint32_t>(rows()))};
      bool onBody = false;
      for (const auto& b : body_)
        if (b == c) onBody = true;
      if (!onBody) {
        food_ = c;
        return;
      }
    }
  }

  static constexpr int kCell = 8;
  static constexpr int kTop = 24;
  static constexpr int kBottom = 16;
  static constexpr DifficultyRamp kPaceRamp{150.0f, -2.5f, 85.0f, 3.0f};

  std::vector<Cell> body_;
  Cell food_{0, 0};
  int dir_ = 1;
  int turn_ = 0;
  int grow_ = 0;
  uint32_t moveAcc_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(snake, SnakeScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_SNAKE
