#include "brand.gen.h"
#if TAMA_GAME_DELIVERY

#include <vector>

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

struct Entity {
  int lane;
  float y;
  bool package;
};

class DeliveryScreen : public ArcadeGameScreen {
 public:
  DeliveryScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0x51ed2701u); }
  const char* id() const override { return "game.delivery"; }

 protected:
  const char* title() const override { return "DELIVERY"; }
  const char* readyHint() const override { return "B SHIFTS LANE"; }
  const char* runHint() const override { return nullptr; }
  const char* deadTitle() const override { return "CRASHED"; }
  const char* hintA() const override { return st_ == St::Run ? nullptr : "GO"; }
  const char* hintB() const override { return "SHIFT"; }

  void renderWorld(Gfx& g, ShellContext& ctx) override {
    auto& c = g.c();

    for (int i = 1; i < kLanes; ++i) {
      const int x = laneEdge(i);
      for (int y = (scroll_ % 18) - 18; y < h_; y += 18) {
        c.drawFastVLine(x, y, 9, theme::kDimmer);
      }
    }

    for (const auto& e : ents_) {
      const int cx = laneCenter(e.lane);
      const int y = static_cast<int>(e.y);
      if (e.package) {
        c.fillRect(cx - 6, y - 6, 12, 12, theme::kHi);
        c.drawRect(cx - 6, y - 6, 12, 12, theme::kFg);
        c.drawFastHLine(cx - 6, y, 12, theme::kDim);
        c.drawFastVLine(cx, y - 6, 12, theme::kDim);
      } else {
        c.fillTriangle(cx, y - 9, cx - 7, y + 7, cx + 7, y + 7, theme::kWarn);
        c.drawFastHLine(cx - 4, y - 1, 8, theme::kInk);
      }
    }

    player(g, ctx, laneCenter(lane_), h_ - 40, 30, Expr::Neutral);
  }

  Transition onAction(Intent intent, ShellContext&) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      lane_ = cycleIndex(intent, lane_, kLanes);
      return Transition::redraw();
    }
    return Transition::none();
  }

  void onReset() override {
    lane_ = 1;
    speed_ = kSpeedRamp.base;
    scroll_ = 0;
    spawnAcc_ = 0;
    lastFree_ = 1;
    ents_.clear();
  }

  void step(ShellContext&) override {
    updateWorld(elapsedSec());
    updateEntities();
  }

 private:
  void updateWorld(float sec) {
    speed_ = kSpeedRamp.at(sec);
    scroll_ = (scroll_ + static_cast<int>(speed_ + 1)) % 18;

    spawnAcc_ += speed_;
    if (spawnAcc_ >= kRowGapRamp.at(sec)) {
      spawnAcc_ = 0;
      spawnWave(sec);
    }
  }

  // Guarantees a reachable open lane each wave: early game blocks one lane (two open),
  // later blocks all but the lane adjacent to the previous gap, so it is never a wall.
  void spawnWave(float sec) {
    const int delta = static_cast<int>(rng_.next() % 3u) - 1;
    int free = lastFree_ + delta;
    if (free < 0) free = 0;
    if (free > kLanes - 1) free = kLanes - 1;
    lastFree_ = free;

    const bool blockBoth = sec - kGrace > 16.0f;
    int chosen = -1;
    if (!blockBoth) {
      int others[kLanes];
      int cnt = 0;
      for (int l = 0; l < kLanes; ++l)
        if (l != free) others[cnt++] = l;
      chosen = others[rng_.next() % static_cast<uint32_t>(cnt)];
    }
    for (int l = 0; l < kLanes; ++l) {
      if (l == free) continue;
      if (blockBoth || l == chosen) ents_.push_back(Entity{l, -10.0f, false});
    }
    if ((rng_.next() % 3u) == 0u) ents_.push_back(Entity{free, -10.0f, true});
  }

  void updateEntities() {
    const int py = h_ - 40;
    for (auto it = ents_.begin(); it != ents_.end();) {
      it->y += speed_;
      const bool hit = it->lane == lane_ && it->y > py - 12 && it->y < py + 12;
      if (hit) {
        if (it->package) {
          ++score_;
          it = ents_.erase(it);
          continue;
        }
        die();
        return;
      }
      if (it->y > h_ + 10) {
        it = ents_.erase(it);
      } else {
        ++it;
      }
    }
  }

  int laneCenter(int i) const { return (w_ * i) / kLanes + w_ / (2 * kLanes); }
  int laneEdge(int i) const { return (w_ * i) / kLanes; }

  static constexpr int kLanes = 3;
  static constexpr float kGrace = 6.0f;
  static constexpr DifficultyRamp kSpeedRamp{1.0f, 0.03f, 3.4f, kGrace};
  static constexpr DifficultyRamp kRowGapRamp{82.0f, -0.5f, 56.0f, kGrace};

  int lane_ = 1;
  float speed_ = kSpeedRamp.base;
  float spawnAcc_ = 0;
  int lastFree_ = 1;
  int scroll_ = 0;
  std::vector<Entity> ents_;
};

}  // namespace

TAMA_SCREEN_FACTORY(delivery, DeliveryScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_DELIVERY
