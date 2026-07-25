#include "ble/hid.h"
#if defined(TAMA_ENABLE_BLE) && TAMA_NEEDS_HID

#include <utility>
#include <vector>

#include "hid_modes.gen.h"

namespace tama {

namespace {

constexpr char kHidServiceUuid[] = "1812";
constexpr uint16_t kGenericHidAppearance = 0x03C0;
constexpr uint8_t kUsbSig = 0x02;
constexpr uint16_t kVendorId = 0x303A;
constexpr uint16_t kProductId = 0x0001;
constexpr uint16_t kProductVersion = 0x0100;

constexpr uint8_t kReportIds[kHidCapabilityCount] = {1, 2, 3, 4};

constexpr uint8_t reportId(HidCapability capability) { return kReportIds[hidIndex(capability)]; }

constexpr uint8_t kGamepadDesc[] = {
    0x05, 0x01,              // Usage Page (Generic Desktop)
    0x09, 0x05,              // Usage (Game Pad)
    0xA1, 0x01,              // Collection (Application)
    0x85, reportId(HidCapability::Gamepad),  //   Report ID
    0x05, 0x09,              //   Usage Page (Button)
    0x19, 0x01,              //   Usage Minimum (1)
    0x29, 0x02,              //   Usage Maximum (2)
    0x15, 0x00,              //   Logical Minimum (0)
    0x25, 0x01,              //   Logical Maximum (1)
    0x75, 0x01,              //   Report Size (1)
    0x95, 0x02,              //   Report Count (2)
    0x81, 0x02,              //   Input (Data, Variable, Absolute)
    0x75, 0x06,              //   Report Size (6) -- pad to a byte
    0x95, 0x01,              //   Report Count (1)
    0x81, 0x03,              //   Input (Constant)
    0x05, 0x01,              //   Usage Page (Generic Desktop)
    0x09, 0x30,              //   Usage (X)
    0x09, 0x31,              //   Usage (Y)
    0x15, 0x81,              //   Logical Minimum (-127)
    0x25, 0x7F,              //   Logical Maximum (127)
    0x75, 0x08,              //   Report Size (8)
    0x95, 0x02,              //   Report Count (2)
    0x81, 0x02,              //   Input (Data, Variable, Absolute)
    0xC0,                    // End Collection
};

constexpr uint8_t kMediaDesc[] = {
    0x05, 0x0C,               // Usage Page (Consumer)
    0x09, 0x01,               // Usage (Consumer Control)
    0xA1, 0x01,               // Collection (Application)
    0x85, reportId(HidCapability::Media),  //   Report ID
    0x15, 0x00,               //   Logical Minimum (0)
    0x26, 0xFF, 0x03,         //   Logical Maximum (0x3FF)
    0x19, 0x00,               //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,         //   Usage Maximum (0x3FF)
    0x75, 0x10,               //   Report Size (16)
    0x95, 0x01,               //   Report Count (1)
    0x81, 0x00,               //   Input (Data, Array)
    0xC0,                     // End Collection
};

constexpr uint8_t kKeyboardDesc[] = {
    0x05, 0x01,               // Usage Page (Generic Desktop)
    0x09, 0x06,               // Usage (Keyboard)
    0xA1, 0x01,               // Collection (Application)
    0x85, reportId(HidCapability::Keyboard),  //   Report ID
    0x05, 0x07,               //   Usage Page (Keyboard)
    0x19, 0xE0,               //   Usage Minimum (Left Control)
    0x29, 0xE7,               //   Usage Maximum (Right GUI)
    0x15, 0x00,               //   Logical Minimum (0)
    0x25, 0x01,               //   Logical Maximum (1)
    0x75, 0x01,               //   Report Size (1)
    0x95, 0x08,               //   Report Count (8)
    0x81, 0x02,               //   Input (Data, Variable, Absolute)
    0x75, 0x08,               //   Report Size (8)
    0x95, 0x01,               //   Report Count (1)
    0x81, 0x03,               //   Input (Constant)
    0x05, 0x07,               //   Usage Page (Keyboard)
    0x19, 0x00,               //   Usage Minimum (0)
    0x29, 0x65,               //   Usage Maximum (101)
    0x15, 0x00,               //   Logical Minimum (0)
    0x25, 0x65,               //   Logical Maximum (101)
    0x75, 0x08,               //   Report Size (8)
    0x95, 0x06,               //   Report Count (6)
    0x81, 0x00,               //   Input (Data, Array)
    0xC0,                     // End Collection
};

constexpr uint8_t kMouseDesc[] = {
    0x05, 0x01,            // Usage Page (Generic Desktop)
    0x09, 0x02,            // Usage (Mouse)
    0xA1, 0x01,            // Collection (Application)
    0x85, reportId(HidCapability::Mouse),  //   Report ID
    0x09, 0x01,            //   Usage (Pointer)
    0xA1, 0x00,            //   Collection (Physical)
    0x05, 0x09,            //     Usage Page (Button)
    0x19, 0x01,            //     Usage Minimum (1)
    0x29, 0x03,            //     Usage Maximum (3)
    0x15, 0x00,            //     Logical Minimum (0)
    0x25, 0x01,            //     Logical Maximum (1)
    0x75, 0x01,            //     Report Size (1)
    0x95, 0x03,            //     Report Count (3)
    0x81, 0x02,            //     Input (Data, Variable, Absolute)
    0x75, 0x05,            //     Report Size (5) -- pad to a byte
    0x95, 0x01,            //     Report Count (1)
    0x81, 0x03,            //     Input (Constant)
    0x05, 0x01,            //     Usage Page (Generic Desktop)
    0x09, 0x30,            //     Usage (X)
    0x09, 0x31,            //     Usage (Y)
    0x15, 0x81,            //     Logical Minimum (-127)
    0x25, 0x7F,            //     Logical Maximum (127)
    0x75, 0x08,            //     Report Size (8)
    0x95, 0x02,            //     Report Count (2)
    0x81, 0x06,            //     Input (Data, Variable, Relative)
    0xC0,                  //   End Collection
    0xC0,                  // End Collection
};

struct CapabilitySpec {
  uint16_t appearance;
  const uint8_t* desc;
  size_t len;
};

constexpr CapabilitySpec kCapabilities[kHidCapabilityCount] = {
    {0x03C4, kGamepadDesc, sizeof(kGamepadDesc)},
    {kGenericHidAppearance, kMediaDesc, sizeof(kMediaDesc)},
    {0x03C1, kKeyboardDesc, sizeof(kKeyboardDesc)},
    {0x03C2, kMouseDesc, sizeof(kMouseDesc)},
};

}  // namespace

HidEndpoint::HidEndpoint(std::string manufacturer, IHidProfileRepository& profiles)
    : manufacturer_(std::move(manufacturer)), profiles_(profiles) {}

const char* HidEndpoint::serviceUuid() const { return kHidServiceUuid; }

uint16_t HidEndpoint::appearance() const {
  for (size_t i = 0; i < kHidCapabilityCount; ++i) {
    if (profile_.has(static_cast<HidCapability>(i))) return kCapabilities[i].appearance;
  }
  return kGenericHidAppearance;
}

void HidEndpoint::setup(BleBearer&, NimBLEServer* nim) {
  profile_ = hid::resolve(profiles_.load());

  std::vector<uint8_t> map;
  for (size_t i = 0; i < kHidCapabilityCount; ++i) {
    if (!profile_.has(static_cast<HidCapability>(i))) continue;
    map.insert(map.end(), kCapabilities[i].desc, kCapabilities[i].desc + kCapabilities[i].len);
  }

  hid_ = new NimBLEHIDDevice(nim);
  hid_->setManufacturer(manufacturer_);
  hid_->setPnp(kUsbSig, kVendorId, kProductId, kProductVersion);
  hid_->setHidInfo(0x00, 0x01);
  hid_->setReportMap(map.data(), map.size());

  for (size_t i = 0; i < kHidCapabilityCount; ++i) {
    const auto capability = static_cast<HidCapability>(i);
    if (!profile_.has(capability)) continue;
    reports_[i] = hid_->getInputReport(reportId(capability));
    reports_[i]->setCallbacks(this);
  }
}

void HidEndpoint::onLink(bool connected) {
  if (!connected) subscribed_ = HidCapabilitySet{};
}

void HidEndpoint::onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo&, uint16_t subValue) {
  for (size_t i = 0; i < kHidCapabilityCount; ++i) {
    if (reports_[i] != chr) continue;
    const auto capability = static_cast<HidCapability>(i);
    if (subValue != 0) {
      subscribed_.add(capability);
    } else {
      subscribed_.remove(capability);
    }
    return;
  }
}

void HidEndpoint::activate() { active_ = true; }

void HidEndpoint::deactivate() {
  if (active_) push(GamepadFrame{});
  active_ = false;
}

bool HidEndpoint::ready(HidCapability capability) const { return subscribed_.has(capability); }

void HidEndpoint::send(const GamepadFrame& frame) {
  if (!active_ || frame == last_) return;
  push(frame);
}

void HidEndpoint::tap(MediaKey key) {
  if (!active_) return;
  const uint16_t usage = static_cast<uint16_t>(key);
  const uint8_t press[2] = {static_cast<uint8_t>(usage & 0xFF),
                            static_cast<uint8_t>(usage >> 8)};
  const uint8_t release[2] = {0, 0};
  notify(HidCapability::Media, press, sizeof(press));
  notify(HidCapability::Media, release, sizeof(release));
}

void HidEndpoint::tap(KeyboardKey key) {
  if (!active_) return;
  const uint8_t press[8] = {0, 0, static_cast<uint8_t>(key), 0, 0, 0, 0, 0};
  const uint8_t release[8] = {};
  notify(HidCapability::Keyboard, press, sizeof(press));
  notify(HidCapability::Keyboard, release, sizeof(release));
}

void HidEndpoint::nudge(int8_t dx, int8_t dy) {
  if (!active_) return;
  const uint8_t report[3] = {0, static_cast<uint8_t>(dx), static_cast<uint8_t>(dy)};
  notify(HidCapability::Mouse, report, sizeof(report));
}

void HidEndpoint::push(const GamepadFrame& frame) {
  last_ = frame;
  const uint8_t report[3] = {frame.buttons, static_cast<uint8_t>(frame.x),
                             static_cast<uint8_t>(frame.y)};
  notify(HidCapability::Gamepad, report, sizeof(report));
}

void HidEndpoint::notify(HidCapability capability, const uint8_t* data, size_t len) {
  auto* chr = reports_[hidIndex(capability)];
  if (!chr || !ready(capability)) return;
  chr->setValue(data, len);
  chr->notify(data, len);
}

}  // namespace tama

#endif
