#include "brand.gen.h"
#if TAMA_GAME_DELIVERY

#include <string>
#include <vector>

#include "arcade.h"
#include "games.h"
#include "mascot.h"
#include "widgets.h"

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

  void render(Gfx& g, ShellContext& ctx) override {
    w_ = g.w();
    h_ = g.h();
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

    const int py = h_ - 40;
    const Expr e = st_ == St::Dead ? Expr::Alert : Expr::Neutral;
    if (ctx.character) {
      ctx.character->draw(g, laneCenter(lane_), py, 30, MascotState{e, 0, true}, now_);
    }

    g.str(std::to_string(score_).c_str(), w_ - 6, 4, theme::kHi, typeface::title(),
          textdatum_t::top_right);

    if (st_ == St::Ready) {
      banner(g, "DELIVERY", "B SHIFTS LANE");
    } else if (st_ == St::Dead) {
      banner(g, "CRASHED", ("BEST " + std::to_string(best_)).c_str());
    }
    widgets::hints(g, st_ == St::Ready || st_ == St::Dead ? "GO" : nullptr, "SHIFT");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (st_ == St::Run) {
      if (intent == Intent::Next || intent == Intent::Prev) {
        lane_ = cycleIndex(intent, lane_, kLanes);
        return Transition::redraw();
      }
      return Transition::none();
    }
    if (intent == Intent::Select) {
      begin();
      return Transition::redraw();
    }
    return Transition::none();
  }

 protected:
  void onReset() override {
    lane_ = 1;
    speed_ = kSpeed0;
    scroll_ = 0;
    spawnAcc_ = 0;
    lastFree_ = 1;
    ents_.clear();
  }

  void step(ShellContext&) override {
    updateWorld(effSec(kGrace));
    updateEntities();
  }

 private:
  void updateWorld(float eff) {
    speed_ = kSpeed0 + eff * 0.03f;
    if (speed_ > 3.4f) speed_ = 3.4f;
    scroll_ = (scroll_ + static_cast<int>(speed_ + 1)) % 18;

    float rowGap = 82.0f - eff * 0.5f;
    if (rowGap < 56.0f) rowGap = 56.0f;
    spawnAcc_ += speed_;
    if (spawnAcc_ >= rowGap) {
      spawnAcc_ = 0;
      spawnWave(eff);
    }
  }

  // Guarantees a reachable open lane each wave: early game blocks one lane (two open),
  // later blocks all but the lane adjacent to the previous gap, so it is never a wall.
  void spawnWave(float eff) {
    const int delta = static_cast<int>(rng_.next() % 3u) - 1;
    int free = lastFree_ + delta;
    if (free < 0) free = 0;
    if (free > kLanes - 1) free = kLanes - 1;
    lastFree_ = free;

    const bool blockBoth = eff > 16.0f;
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
  static constexpr float kSpeed0 = 1.0f;
  static constexpr float kGrace = 6.0f;

  int lane_ = 1;
  float speed_ = kSpeed0;
  float spawnAcc_ = 0;
  int lastFree_ = 1;
  int scroll_ = 0;
  std::vector<Entity> ents_;
};

}  // namespace

AppScreen& delivery() {
  static DeliveryScreen instance;
  return instance;
}

}  // namespace tama::games

#endif  // TAMA_GAME_DELIVERY
