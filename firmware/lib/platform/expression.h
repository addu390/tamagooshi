#pragma once

#include "model.h"

namespace tama {

class IExpressionSink {
 public:
  virtual ~IExpressionSink() = default;
  virtual void begin() = 0;
  virtual void apply(const ExpressionState& state) = 0;
  virtual void play(const ExpressionCue& cue) = 0;
};

}  // namespace tama
