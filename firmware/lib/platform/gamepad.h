#pragma once

#include <cstdint>

namespace tama {

struct GamepadFrame {
  int8_t x = 0;
  int8_t y = 0;
  uint8_t buttons = 0;

  bool operator==(const GamepadFrame& other) const {
    return x == other.x && y == other.y && buttons == other.buttons;
  }

  bool operator!=(const GamepadFrame& other) const { return !(*this == other); }
};

class IGamepadLink {
 public:
  virtual ~IGamepadLink() = default;
  virtual void activate() = 0;
  virtual void deactivate() = 0;
  virtual bool ready() const = 0;
  virtual void send(const GamepadFrame& frame) = 0;
};

}  // namespace tama
