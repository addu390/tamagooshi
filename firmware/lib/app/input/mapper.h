#pragma once

#include <functional>

#include "input.h"
#include "intent.h"

namespace tama {

class InputMapper {
 public:
  using IntentHandler = std::function<void(Intent)>;

  InputMapper(IButtonSource& buttons, IInputSource& input, const DeviceCapabilities& caps);

  void begin();
  void poll();
  void onIntent(IntentHandler handler);

 private:
  void onButton(ButtonEvent event);
  void onJoystick(const JoystickEvent& event);

  IButtonSource& buttons_;
  IInputSource& input_;
  const DeviceCapabilities& caps_;
  IntentHandler handler_;
};

}  // namespace tama
