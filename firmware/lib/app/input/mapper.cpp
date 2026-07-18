#include "mapper.h"

#include <utility>

namespace tama {

InputMapper::InputMapper(IButtonSource& buttons, IInputSource& input,
                         const DeviceCapabilities& caps)
    : buttons_(buttons), input_(input), caps_(caps) {}

void InputMapper::onIntent(IntentHandler handler) { handler_ = std::move(handler); }

void InputMapper::begin() {
  buttons_.onEvent([this](ButtonEvent e) { onButton(e); });
  input_.onJoystick([this](const JoystickEvent& e) { onJoystick(e); });
  buttons_.begin();
  input_.begin();
}

void InputMapper::poll() {
  buttons_.poll();
  input_.poll();
}

void InputMapper::onButton(ButtonEvent event) {
  if (!handler_) return;
  switch (event) {
    case ButtonEvent::AClick: handler_(Intent::Select); break;
    case ButtonEvent::BClick: handler_(Intent::Next); break;
    case ButtonEvent::AHold: handler_(Intent::Home); break;
    case ButtonEvent::BHold: handler_(Intent::Back); break;
    case ButtonEvent::ABHold: break;
  }
}

void InputMapper::onJoystick(const JoystickEvent& event) {
  if (!handler_ || !caps_.joystick) return;
  if (event.pressed) {
    handler_(Intent::Select);
  } else if (event.y < -50 || event.x < -50) {
    handler_(Intent::Prev);
  } else if (event.y > 50 || event.x > 50) {
    handler_(Intent::Next);
  }
}

}  // namespace tama
