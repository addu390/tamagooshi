#pragma once

#include <cstdint>
#include <vector>

#include "screen.h"
#include "rng.h"
#include "theme.h"

namespace tama::games {

struct DifficultyRamp {
  float base;
  float perSec;
  float maxV;
  float grace;

  float at(float sec) const {
    const float eff = sec < grace ? 0.0f : sec - grace;
    const float v = base + eff * perSec;
    return v > maxV ? maxV : v;
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

class ArcadeGameScreen : public AppScreen {
 public:
  explicit ArcadeGameScreen(OrientationPref orient) : orient_(orient) {}

  OrientationPref orientation() const override { return orient_; }
  void onEnter(ShellContext&) override { toReady(); }

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
    }
    return Transition::redraw();
  }

 protected:
  enum class St { Ready, Run, Dead };

  virtual void onReset() = 0;
  virtual void step(ShellContext& ctx) = 0;

  void begin() {
    onReset();
    score_ = 0;
    start_ = 0;
    st_ = St::Run;
  }

  void die() {
    st_ = St::Dead;
    if (score_ > best_) best_ = score_;
  }

  float elapsedSec() const { return start_ == 0 ? 0.0f : (now_ - start_) / 1000.0f; }
  float effSec(float grace) const {
    const float s = elapsedSec();
    return s < grace ? 0.0f : s - grace;
  }

  void banner(Gfx& g, const char* title, const char* sub) {
    const int cx = w_ / 2;
    g.str(title, cx, h_ / 2 - 14, theme::kHi, typeface::title(), textdatum_t::middle_center);
    g.str(sub, cx, h_ / 2 + 10, theme::kDim, typeface::body(), textdatum_t::middle_center);
  }

  St st_ = St::Ready;
  int score_ = 0;
  int best_ = 0;
  int w_ = 135;
  int h_ = 240;
  uint32_t now_ = 0;
  uint32_t start_ = 0;
  LcgRng rng_{0x1234abcdu};

 private:
  void toReady() {
    onReset();
    score_ = 0;
    start_ = 0;
    last_ = 0;
    acc_ = 0;
    st_ = St::Ready;
  }

  static constexpr uint32_t kStepMs = 16;

  uint32_t last_ = 0;
  uint32_t acc_ = 0;
  OrientationPref orient_;
};

}  // namespace tama::games
