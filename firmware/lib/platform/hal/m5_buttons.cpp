#include "m5_buttons.h"

#include <M5Unified.h>

namespace tama {

void M5Buttons::begin() {}

void M5Buttons::onEvent(Handler handler) { handler_ = std::move(handler); }

void M5Buttons::poll() {
  if (!handler_) return;

  if (M5.BtnA.isHolding() && M5.BtnB.isHolding()) {
    if (!comboLatched_) {
      comboLatched_ = true;
      handler_(ButtonEvent::ABHold);
    }
    return;
  }
  if (!M5.BtnA.isPressed() && !M5.BtnB.isPressed()) comboLatched_ = false;
  if (comboLatched_) return;

  if (M5.BtnA.wasHold()) {
    handler_(ButtonEvent::AHold);
  } else if (M5.BtnA.wasClicked()) {
    handler_(ButtonEvent::AClick);
  }
  if (M5.BtnB.wasHold()) {
    handler_(ButtonEvent::BHold);
  } else if (M5.BtnB.wasClicked()) {
    handler_(ButtonEvent::BClick);
  }
}

}  // namespace tama
