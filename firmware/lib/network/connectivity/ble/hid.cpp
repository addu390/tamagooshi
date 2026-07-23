#include "ble/hid.h"
#if defined(TAMA_ENABLE_BLE)

#include <utility>

namespace tama {

namespace {

constexpr char kHidServiceUuid[] = "1812";
constexpr uint16_t kGamepadAppearance = 0x03C4;
constexpr uint8_t kReportId = 1;
constexpr uint8_t kUsbSig = 0x02;
constexpr uint16_t kVendorId = 0x303A;
constexpr uint16_t kProductId = 0x0001;
constexpr uint16_t kProductVersion = 0x0100;

// Gamepad report: 2 buttons packed into one byte, then X/Y as signed bytes.
constexpr uint8_t kReportMap[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, kReportId,   //   Report ID
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x02,        //   Usage Maximum (2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0x75, 0x06,        //   Report Size (6) -- pad to a byte
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x03,        //   Input (Constant)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x15, 0x81,        //   Logical Minimum (-127)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0xC0,              // End Collection
};

}  // namespace

GamepadEndpoint::GamepadEndpoint(std::string manufacturer)
    : manufacturer_(std::move(manufacturer)) {}

const char* GamepadEndpoint::serviceUuid() const { return kHidServiceUuid; }

uint16_t GamepadEndpoint::appearance() const { return kGamepadAppearance; }

void GamepadEndpoint::setup(BleBearer&, NimBLEServer* nim) {
  hid_ = new NimBLEHIDDevice(nim);
  hid_->setManufacturer(manufacturer_);
  hid_->setPnp(kUsbSig, kVendorId, kProductId, kProductVersion);
  hid_->setHidInfo(0x00, 0x01);
  hid_->setReportMap(const_cast<uint8_t*>(kReportMap), sizeof(kReportMap));

  input_ = hid_->getInputReport(kReportId);
  input_->setCallbacks(this);
}

void GamepadEndpoint::onLink(bool connected) {
  if (!connected) subscribed_ = false;
}

void GamepadEndpoint::onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo&,
                                  uint16_t subValue) {
  if (chr == input_) subscribed_ = subValue != 0;
}

void GamepadEndpoint::activate() { active_ = true; }

void GamepadEndpoint::deactivate() {
  if (active_) push(GamepadFrame{});
  active_ = false;
}

bool GamepadEndpoint::ready() const { return subscribed_; }

void GamepadEndpoint::send(const GamepadFrame& frame) {
  if (!active_ || frame == last_) return;
  push(frame);
}

void GamepadEndpoint::push(const GamepadFrame& frame) {
  last_ = frame;
  if (!input_ || !subscribed_) return;

  uint8_t report[3] = {frame.buttons, static_cast<uint8_t>(frame.x),
                       static_cast<uint8_t>(frame.y)};
  input_->setValue(report, sizeof(report));
  input_->notify();
}

}  // namespace tama

#endif
