#pragma once

#include <cstdint>

#include "gfx.h"
#include "mascot.h"

namespace tama {

class Character {
 public:
  virtual ~Character() = default;
  virtual const char* id() const = 0;
  virtual const char* name() const = 0;
  virtual const char* category() const = 0;
  virtual void draw(Gfx& g, int cx, int cy, int size, const MascotState& st,
                    uint32_t tickMs) = 0;
};

}  // namespace tama
