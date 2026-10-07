#pragma once

#include <algorithm>
#include <cstdint>

#include "gfx.h"
#include "mascot.h"
#include "theme.h"

namespace tama {

class Character {
 public:
  virtual ~Character() = default;
  virtual const char* id() const = 0;
  virtual const char* name() const = 0;
  virtual const char* category() const = 0;
  virtual void draw(Gfx& g, int cx, int cy, int size, const MascotState& st,
                    uint32_t tickMs) = 0;

 protected:
  static void drawShadow(Gfx& g, int cx, int groundY, int width, int base, int lift) {
    const int rx = std::max(3, width / 4 - lift);
    const int ry = std::max(1, base - lift / 4);
    g.c().fillEllipse(cx, groundY + 2, rx, ry, theme::kDimmer);
  }
};

}  // namespace tama
