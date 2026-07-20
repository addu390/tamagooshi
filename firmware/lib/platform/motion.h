#pragma once

#include <cmath>
#include <cstdint>

#include "input.h"

namespace tama::motion {

constexpr float kMinMagnitude = 0.5f;
constexpr float kFlatZ = 0.75f;
constexpr float kAxisPull = 0.6f;

class TiltFilter {
 public:
  explicit TiltFilter(float smoothing) : smoothing_(smoothing) {}

  bool sample(ISensorSource* sensor, bool reset = false) {
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    if (!sensor || !sensor->accel(ax, ay, az)) return false;
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (mag < kMinMagnitude) return false;
    ax /= mag;
    ay /= mag;
    az /= mag;
    if (reset || !ready_) {
      x_ = ax;
      y_ = ay;
      z_ = az;
      ready_ = true;
    } else {
      x_ += (ax - x_) * smoothing_;
      y_ += (ay - y_) * smoothing_;
      z_ += (az - z_) * smoothing_;
    }
    return true;
  }

  void reset() { ready_ = false; }
  bool ready() const { return ready_; }
  float x() const { return x_; }
  float y() const { return y_; }
  float z() const { return z_; }

 private:
  float smoothing_;
  bool ready_ = false;
  float x_ = 0.0f;
  float y_ = 0.0f;
  float z_ = 1.0f;
};

enum class Posture : uint8_t { None, Upright, Flipped, Flat, TiltLeft, TiltRight };

inline Posture classify(const TiltFilter& f) {
  if (std::fabs(f.z()) > kFlatZ) return Posture::Flat;
  if (std::fabs(f.y()) > kAxisPull) return f.y() > 0 ? Posture::Upright : Posture::Flipped;
  if (std::fabs(f.x()) > kAxisPull) return f.x() > 0 ? Posture::TiltRight : Posture::TiltLeft;
  return Posture::None;
}

inline bool isTilt(Posture p) { return p == Posture::TiltLeft || p == Posture::TiltRight; }

// Debounces raw classification: a posture must hold for its dwell time before
// it is reported through `out`.
class PostureTracker {
 public:
  PostureTracker(uint32_t dwellMs, uint32_t flipDwellMs)
      : dwellMs_(dwellMs), flipDwellMs_(flipDwellMs) {}

  bool settle(const TiltFilter& f, uint32_t nowMs, Posture& out) {
    const Posture p = classify(f);
    if (p != candidate_) {
      candidate_ = p;
      since_ = nowMs;
      return false;
    }
    const uint32_t dwell = p == Posture::Flipped ? flipDwellMs_ : dwellMs_;
    if (nowMs - since_ < dwell) return false;
    out = p;
    return true;
  }

  void reset() { candidate_ = Posture::None; }

 private:
  uint32_t dwellMs_;
  uint32_t flipDwellMs_;
  Posture candidate_ = Posture::None;
  uint32_t since_ = 0;
};

}  // namespace tama::motion
