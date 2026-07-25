#pragma once

#include "input.h"

namespace tama {

class M5Buttons : public IButtonSource {
 public:
  void begin() override;
  void poll() override;
  void onEvent(Handler handler) override;
  bool held(int index) const override;

 private:
  Handler handler_;
  bool comboLatched_ = false;
};

}  // namespace tama
