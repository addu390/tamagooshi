#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mascot.h"
#include "rng.h"
#include "screen.h"
#include "theme.h"
#include "widgets.h"

namespace tama::games {

// Linear ramp from base toward limit, in either direction, after a grace period.
struct DifficultyRamp {
  float base;
  float perSec;
  float limit;
  float grace;

  float at(float sec) const {
    const float eff = sec < grace ? 0.0f : sec - grace;
    const float v = base + eff * perSec;
    if (perSec >= 0) return v > limit ? limit : v;
    return v < limit ? limit : v;
  }
};

struct Body1D {
  float p = 0.0f;
  float v = 0.0f;

  void integrate(float accel, float dt) {
    v += accel * dt;
    p += v * dt;
  }

  void rest(float at) {
    p = at;
    v = 0.0f;
  }
};

template <class T>
struct HorizSpawner {
  std::vector<T> items;

  void clear() { items.clear(); }
  void advance(float dx) {
    for (auto& it : items) it.x -= dx;
  }
  float rightmost(float dflt) const {
    float r = dflt;
    for (const auto& it : items)
      if (it.x > r) r = it.x;
    return r;
  }
  bool due(float threshold, float dflt) const {
    return items.empty() || rightmost(dflt) < threshold;
  }
};

// One wall with a gap, scrolling right to left and respawning at the right edge.
class GapChannel {
 public:
  GapChannel(int wallW, int gapH, int margin) : wallW_(wallW), gapH_(gapH), margin_(margin) {}

  void reset(LcgRng& rng, int fieldW, int fieldH) {
    x_ = static_cast<float>(fieldW);
    roll(rng, fieldH);
  }

  bool advance(float dx, LcgRng& rng, int fieldW, int fieldH) {
    x_ -= dx;
    if (x_ >= -wallW_) return false;
    reset(rng, fieldW, fieldH);
    return true;
  }

  bool blocks(float px, float py, float r) const {
    const int wx = static_cast<int>(x_);
    if (px + r <= wx || px - r >= wx + wallW_) return false;
    return py - r < gapY_ || py + r > gapY_ + gapH_;
  }

  bool cleared(float px, float r) const { return static_cast<int>(x_) + wallW_ < px - r; }

  void draw(M5Canvas& c, int fieldH) const {
    const int wx = static_cast<int>(x_);
    c.fillRect(wx, 0, wallW_, gapY_, theme::kDimmer);
    c.drawRect(wx, 0, wallW_, gapY_, theme::kFg);
    const int bot = gapY_ + gapH_;
    c.fillRect(wx, bot, wallW_, fieldH - bot, theme::kDimmer);
    c.drawRect(wx, bot, wallW_, fieldH - bot, theme::kFg);
  }

 private:
  void roll(LcgRng& rng, int fieldH) {
    const int span = fieldH - gapH_ - 2 * margin_;
    gapY_ = margin_ + static_cast<int>(rng.next() % static_cast<uint32_t>(span > 1 ? span : 1));
  }

  int wallW_;
  int gapH_;
  int margin_;
  float x_ = 0.0f;
  int gapY_ = 0;
};

// Fixed-step game loop with shared chrome: score, ready and dead banners,
// button hints, mascot player, and death feedback. Games implement the world.
class ArcadeGameScreen : public AppScreen {
 public:
  explicit ArcadeGameScreen(OrientationPref orient) : orient_(orient) {}

  OrientationPref orientation() const override { return orient_; }
  void onEnter(ShellContext&) override { toReady(); }

  void render(Gfx& g, ShellContext& ctx) final {
    w_ = g.w();
    h_ = g.h();
    renderWorld(g, ctx);
    chrome(g);
  }

  Transition handleInput(Intent intent, ShellContext& ctx) final {
    if (st_ != St::Run) {
      if (intent != Intent::Select) return Transition::none();
      begin();
      return Transition::redraw();
    }
    return onAction(intent, ctx);
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    now_ = nowMs;
    if (last_ == 0) last_ = nowMs;
    uint32_t dt = nowMs - last_;
    last_ = nowMs;
    if (st_ != St::Run) return Transition::redraw();

    if (start_ == 0) start_ = nowMs;
    if (dt > 100) dt = 100;
    acc_ += dt;
    while (acc_ >= kStepMs) {
      acc_ -= kStepMs;
      step(ctx);
      if (st_ == St::Dead) {
        cue(ctx, newBest_ ? ExpressionKind::Celebrate : ExpressionKind::Haptic);
        break;
      }
    }
    return Transition::redraw();
  }

 protected:
  enum class St { Ready, Run, Dead };

  virtual void onReset() = 0;
  virtual void step(ShellContext& ctx) = 0;
  virtual void renderWorld(Gfx& g, ShellContext& ctx) = 0;
  virtual Transition onAction(Intent, ShellContext&) { return Transition::none(); }

  virtual const char* title() const = 0;
  virtual const char* readyHint() const = 0;
  virtual const char* runHint() const = 0;
  virtual const char* deadTitle() const { return "GAME OVER"; }
  virtual const char* hintA() const { return st_ == St::Dead ? "RETRY" : runHint(); }
  virtual const char* hintB() const { return nullptr; }

  void begin() {
    onReset();
    score_ = 0;
    start_ = 0;
    newBest_ = false;
    st_ = St::Run;
  }

  void die() {
    st_ = St::Dead;
    newBest_ = score_ > best_;
    if (newBest_) best_ = score_;
  }

  void player(Gfx& g, ShellContext& ctx, int x, int y, int size, Expr runExpr, int liftPx = 0,
              bool shadow = true) {
    if (!ctx.character) return;
    const Expr e = st_ == St::Dead ? Expr::Alert : runExpr;
    ctx.character->draw(g, x, y, size, MascotState{e, 0, true, liftPx, shadow}, now_);
  }

  void cue(ShellContext& ctx, ExpressionKind kind) {
    if (ctx.expression && !ctx.state.muted) ctx.expression->play({kind, 100, 0});
  }

  float elapsedSec() const { return start_ == 0 ? 0.0f : (now_ - start_) / 1000.0f; }

  St st_ = St::Ready;
  int score_ = 0;
  int best_ = 0;
  int w_ = 135;
  int h_ = 240;
  uint32_t now_ = 0;
  uint32_t start_ = 0;
  LcgRng rng_{0x1234abcdu};

  static constexpr uint32_t kStepMs = 16;
  static constexpr float kStepSec = kStepMs / 1000.0f;

 private:
  void chrome(Gfx& g) {
    g.str(std::to_string(score_).c_str(), w_ / 2, 6, theme::kHi, typeface::title(),
          textdatum_t::top_center);
    if (st_ == St::Ready) {
      banner(g, title(), readyHint());
    } else if (st_ == St::Dead) {
      banner(g, deadTitle(), ("BEST " + std::to_string(best_)).c_str());
    }
    widgets::hints(g, hintA(), hintB());
  }

  void banner(Gfx& g, const char* head, const char* sub) {
    const int cx = w_ / 2;
    g.str(head, cx, h_ / 2 - 14, theme::kHi, typeface::title(), textdatum_t::middle_center);
    g.str(sub, cx, h_ / 2 + 10, theme::kDim, typeface::body(), textdatum_t::middle_center);
  }

  void toReady() {
    onReset();
    score_ = 0;
    start_ = 0;
    last_ = 0;
    acc_ = 0;
    newBest_ = false;
    st_ = St::Ready;
  }

  uint32_t last_ = 0;
  uint32_t acc_ = 0;
  bool newBest_ = false;
  OrientationPref orient_;
};

}  // namespace tama::games
