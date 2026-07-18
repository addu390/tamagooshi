#pragma once

#include <cstdint>

#include "gfx.h"
#include "intent.h"
#include "context.h"
#include "transition.h"

namespace tama {

enum class OrientationPref { Inherit, Portrait, Landscape };

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
  virtual Transition tick(ShellContext& ctx, uint32_t nowMs) {
    (void)ctx;
    (void)nowMs;
    return Transition::none();
  }
};

}  // namespace tama
