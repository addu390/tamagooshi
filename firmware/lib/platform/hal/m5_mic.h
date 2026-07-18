#pragma once

#include <M5Unified.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "input.h"

namespace tama {

class M5Mic : public IMicSource {
 public:
  void begin() override {
    if (M5.Speaker.isEnabled()) M5.Speaker.end();
    M5.Mic.begin();
  }

  void end() override {
    if (M5.Mic.isEnabled()) M5.Mic.end();
    M5.Speaker.begin();
  }

  int level() override {
    if (!M5.Mic.isEnabled()) return 0;

    std::memset(buf_, 0, sizeof(buf_));
    M5.Mic.record(buf_, kSamples, kRate);

    const uint32_t start = m5gfx::millis();
    while (M5.Mic.isRecording()) {
      if (m5gfx::millis() - start > 100) break;
      m5gfx::delay(1);
    }

    int32_t sum = 0;
    for (int i = 0; i < kSamples; ++i) sum += std::abs(buf_[i]);
    return sum / kSamples;
  }

 private:
  static constexpr int kSamples = 256;
  static constexpr uint32_t kRate = 16000;

  int16_t buf_[kSamples] = {0};
};

}  // namespace tama
