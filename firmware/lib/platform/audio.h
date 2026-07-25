#pragma once

#include "input.h"

namespace tama::audio {

class LevelMeter {
 public:
  explicit LevelMeter(int minDelta) : minDelta_(minDelta) {}

  int sample(IMicSource* mic) {
    level_ = mic ? mic->level() : 0;
    if (baseline_ < 0) {
      baseline_ = level_;
    } else {
      baseline_ += (level_ - baseline_) / (level_ < baseline_ ? 4 : 128);
    }
    return level_;
  }

  void reset() {
    level_ = 0;
    baseline_ = -1;
  }

  int level() const { return level_; }
  int baseline() const { return baseline_ < 0 ? 0 : baseline_; }
  int threshold() const { return baseline() * 2 + minDelta_; }
  bool loud() const { return level_ > threshold(); }

 private:
  int minDelta_;
  int level_ = 0;
  int baseline_ = -1;
};

}  // namespace tama::audio
