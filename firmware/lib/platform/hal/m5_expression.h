#pragma once

#include "expression.h"

namespace tama {

class M5Expression : public IExpressionSink {
 public:
  explicit M5Expression(int redLedPin);

  void begin() override;
  void apply(const ExpressionState& state) override;
  void play(const ExpressionCue& cue) override;

 private:
  void led(bool on);

  int redLedPin_;
  bool buzzing_ = false;
  bool ledOn_ = false;
};

}  // namespace tama
