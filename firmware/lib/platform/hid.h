#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace tama {

enum class HidCapability : uint8_t { Gamepad, Media, Keyboard, Mouse };

constexpr size_t kHidCapabilityCount = 4;

constexpr size_t hidIndex(HidCapability capability) { return static_cast<size_t>(capability); }

class HidCapabilitySet {
 public:
  constexpr HidCapabilitySet() = default;
  constexpr explicit HidCapabilitySet(uint8_t bits) : bits_(bits) {}

  constexpr bool has(HidCapability capability) const { return (bits_ & maskOf(capability)) != 0; }
  constexpr bool empty() const { return bits_ == 0; }
  constexpr uint8_t bits() const { return bits_; }

  constexpr void add(HidCapability capability) {
    bits_ = static_cast<uint8_t>(bits_ | maskOf(capability));
  }
  constexpr void remove(HidCapability capability) {
    bits_ = static_cast<uint8_t>(bits_ & ~maskOf(capability));
  }

  constexpr HidCapabilitySet operator&(HidCapabilitySet other) const {
    return HidCapabilitySet(static_cast<uint8_t>(bits_ & other.bits_));
  }

  constexpr bool operator==(HidCapabilitySet other) const { return bits_ == other.bits_; }
  constexpr bool operator!=(HidCapabilitySet other) const { return bits_ != other.bits_; }

 private:
  static constexpr uint8_t maskOf(HidCapability capability) {
    return static_cast<uint8_t>(1u << hidIndex(capability));
  }

  uint8_t bits_ = 0;
};

class IHidProfileRepository {
 public:
  virtual ~IHidProfileRepository() = default;
  virtual std::optional<HidCapabilitySet> load() = 0;
  virtual void save(HidCapabilitySet profile) = 0;
};

class NullHidProfileRepository : public IHidProfileRepository {
 public:
  std::optional<HidCapabilitySet> load() override { return std::nullopt; }
  void save(HidCapabilitySet) override {}
};

struct GamepadFrame {
  int8_t x = 0;
  int8_t y = 0;
  uint8_t buttons = 0;

  bool operator==(const GamepadFrame& other) const {
    return x == other.x && y == other.y && buttons == other.buttons;
  }

  bool operator!=(const GamepadFrame& other) const { return !(*this == other); }
};

enum class MediaKey : uint16_t {
  PlayPause = 0x00CD,
  Next = 0x00B5,
  Prev = 0x00B6,
  VolumeUp = 0x00E9,
  VolumeDown = 0x00EA,
  Mute = 0x00E2,
};

enum class KeyboardKey : uint8_t {
  B = 0x05,
  Period = 0x37,
  PageUp = 0x4B,
  PageDown = 0x4E,
};

inline constexpr uint8_t kMouseBtnLeft = 0x01;
inline constexpr uint8_t kMouseBtnRight = 0x02;
inline constexpr uint8_t kMouseBtnMiddle = 0x04;

class IHidLink {
 public:
  virtual ~IHidLink() = default;
  virtual void activate() = 0;
  virtual void deactivate() = 0;
  virtual bool ready(HidCapability capability) const = 0;
  virtual void send(const GamepadFrame& frame) = 0;
  virtual void tap(MediaKey key) = 0;
  virtual void tap(KeyboardKey key) = 0;
  virtual void pointer(int8_t dx, int8_t dy, uint8_t buttons = 0) = 0;
};

}  // namespace tama
