#pragma once

#include <cstdint>

#include "anim.h"
#include "gfx.h"
#include "intent.h"
#include "context.h"
#include "transition.h"

namespace tama {

enum class OrientationPref { Inherit, Portrait, Landscape, PortraitFlipped };

class AppScreen {
 public:
  virtual ~AppScreen() = default;
  virtual const char* id() const = 0;
  virtual OrientationPref orientation() const { return OrientationPref::Inherit; }
  virtual void onEnter(ShellContext&) {}
  virtual void onExit() {}
  virtual void render(Gfx& g, ShellContext& ctx) = 0;
  virtual Transition handleInput(Intent intent, ShellContext& ctx) {
    (void)intent;
    (void)ctx;
    return Transition::none();
  }
  // Screens that only animate declare a cadence; the default tick handles it.
  virtual uint32_t redrawPeriodMs() const { return 0; }
  virtual Transition tick(ShellContext& ctx, uint32_t nowMs) {
    (void)ctx;
    now_ = nowMs;
    const uint32_t period = redrawPeriodMs();
    if (period == 0) return Transition::none();
    return anim_.due(nowMs, period) ? Transition::redraw() : Transition::none();
  }

 protected:
  uint32_t now() const { return now_; }

 private:
  uint32_t now_ = 0;
  AnimClock anim_;
};

#define TAMA_SCREEN_FACTORY(fn, Class) \
  AppScreen& fn() {                    \
    static Class instance;             \
    return instance;                   \
  }

}  // namespace tama
