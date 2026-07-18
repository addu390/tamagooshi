#pragma once

#include <cstdint>
#include <functional>

#include "model.h"

namespace tama {

enum class ButtonEvent { AClick, AHold, BClick, BHold, ABHold };

class IButtonSource {
 public:
  using Handler = std::function<void(ButtonEvent)>;

  virtual ~IButtonSource() = default;
  virtual void begin() = 0;
  virtual void poll() = 0;
  virtual void onEvent(Handler handler) = 0;
};

class IInputSource {
 public:
  using JoystickHandler = std::function<void(const JoystickEvent&)>;
  using GestureHandler = std::function<void(const GestureEvent&)>;

  virtual ~IInputSource() = default;
  virtual void begin() = 0;
  virtual void poll() = 0;
  virtual void onJoystick(JoystickHandler handler) = 0;
  virtual void onGesture(GestureHandler handler) = 0;
};

class ISensorSource {
 public:
  using MotionHandler = std::function<void(const MotionEvent&)>;

  virtual ~ISensorSource() = default;
  virtual void begin() = 0;
  virtual void poll() = 0;
  virtual void onMotion(MotionHandler handler) = 0;
  virtual float tiltX() { return 0.0f; }
  virtual float tiltY() { return 0.0f; }
};

class IMicSource {
 public:
  virtual ~IMicSource() = default;
  virtual void begin() = 0;
  virtual void end() = 0;
  virtual int level() = 0;
};

class NullInputSource : public IInputSource {
 public:
  void begin() override {}
  void poll() override {}
  void onJoystick(JoystickHandler) override {}
  void onGesture(GestureHandler) override {}
};

class NullMicSource : public IMicSource {
 public:
  void begin() override {}
  void end() override {}
  int level() override { return 0; }
};

class NullSensorSource : public ISensorSource {
 public:
  void begin() override {}
  void poll() override {}
  void onMotion(MotionHandler) override {}
};

}  // namespace tama
