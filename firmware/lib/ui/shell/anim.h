#pragma once

#include <cstdint>

namespace tama {

struct AnimClock {
  uint32_t last = 0;
  bool due(uint32_t now, uint32_t period) {
    if (now - last >= period) {
      last = now;
      return true;
    }
    return false;
  }
};

}  // namespace tama
