#pragma once

#include <cstdint>

namespace tama {

class LcgRng {
 public:
  explicit LcgRng(uint32_t seed = 0x00C0FFEEu) : state_(seed) {}

  void seed(uint32_t s) { state_ = s; }

  uint32_t next() {
    state_ = state_ * 1664525u + 1013904223u;
    return state_ >> 8;
  }

  int range(int n) { return n > 0 ? static_cast<int>(next() % static_cast<uint32_t>(n)) : 0; }

  float unit() { return static_cast<float>(next() % 1000u) / 1000.0f; }

 private:
  uint32_t state_;
};

}  // namespace tama
