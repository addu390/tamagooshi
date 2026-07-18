#pragma once

#include <M5Unified.h>

#include <cmath>
#include <utility>

#include "input.h"

namespace tama {

class M5Imu : public ISensorSource {
 public:
  void begin() override {}
  void onMotion(MotionHandler handler) override { handler_ = std::move(handler); }

  void poll() override {
    if (!handler_) return;
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    if (!M5.Imu.getAccel(&ax, &ay, &az)) return;
    const float magnitude = std::sqrt(ax * ax + ay * ay + az * az);
    const uint32_t now = m5gfx::millis();
    if (magnitude > kShakeG && now - lastShake_ > kCooldownMs) {
      lastShake_ = now;
      handler_(MotionEvent{ax, ay, az, "shake"});
    }
  }

  float tiltX() override {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    return M5.Imu.getAccel(&x, &y, &z) ? x : 0.0f;
  }

  float tiltY() override {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    return M5.Imu.getAccel(&x, &y, &z) ? y : 0.0f;
  }

 private:
  static constexpr float kShakeG = 2.2f;
  static constexpr uint32_t kCooldownMs = 1200;

  MotionHandler handler_;
  uint32_t lastShake_ = 0;
};

}  // namespace tama
