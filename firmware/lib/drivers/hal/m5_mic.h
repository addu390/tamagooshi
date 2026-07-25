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
    powerCodecForCapture();
    M5.Mic.begin();
  }

  void end() override {
    if (M5.Mic.isEnabled()) M5.Mic.end();
    M5.Speaker.begin();
  }

  int level() override {
    if (!M5.Mic.isEnabled()) return 0;

    std::memset(buf_, 0, sizeof(buf_));
    if (!capture(buf_, kSamples)) return 0;

    int32_t mean = 0;
    for (int i = 0; i < kSamples; ++i) mean += buf_[i];
    mean /= kSamples;

    int32_t sum = 0;
    for (int i = 0; i < kSamples; ++i) sum += std::abs(buf_[i] - mean);
    return sum / kSamples;
  }

  bool startRecord() override { return M5.Mic.isEnabled(); }

  size_t readRecord(int16_t* out, size_t maxSamples) override {
    if (!M5.Mic.isEnabled() || maxSamples == 0) return 0;
    size_t n = maxSamples < kSamples ? maxSamples : static_cast<size_t>(kSamples);
    return capture(out, n) ? n : 0;
  }

  void stopRecord() override {}

  uint32_t recordRate() const override { return kRate; }

 private:
  void powerCodecForCapture() {
    if (M5.getBoard() != m5::board_t::board_M5StickS3) return;
    M5.In_I2C.bitOn(kStickS3PmicAddr, kPmicOutputReg, kCodecPowerBit, kPmicFreqHz);
    m5gfx::delay(20);
  }

  bool capture(int16_t* out, size_t samples) {
    if (!M5.Mic.record(out, samples, kRate)) return false;
    const uint32_t start = m5gfx::millis();
    while (M5.Mic.isRecording()) {
      if (m5gfx::millis() - start > 100) return false;
      m5gfx::delay(1);
    }
    return true;
  }

  static constexpr int kSamples = 256;
  static constexpr uint32_t kRate = 16000;
  static constexpr uint8_t kStickS3PmicAddr = 0x6E;
  static constexpr uint8_t kPmicOutputReg = 0x11;
  static constexpr uint8_t kCodecPowerBit = 0b00001000;
  static constexpr uint32_t kPmicFreqHz = 100000;

  int16_t buf_[kSamples] = {0};
};

}  // namespace tama
