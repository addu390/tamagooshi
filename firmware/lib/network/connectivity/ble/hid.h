#pragma once
#if defined(TAMA_ENABLE_BLE) && TAMA_NEEDS_HID

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <cstdint>
#include <string>

#include <hid.h>

#include "ble/ble.h"

namespace tama {

class HidEndpoint : public IHidLink, public IBleService, public NimBLECharacteristicCallbacks {
 public:
  HidEndpoint(std::string manufacturer, IHidProfileRepository& profiles);

  void activate() override;
  void deactivate() override;
  bool ready(HidCapability capability) const override;
  void send(const GamepadFrame& frame) override;
  void tap(MediaKey key) override;
  void tap(KeyboardKey key) override;
  void pointer(int8_t dx, int8_t dy, uint8_t buttons = 0) override;

  void setup(BleBearer& bearer, NimBLEServer* nim) override;
  const char* serviceUuid() const override;
  bool advertiseUuid() const override { return true; }
  uint16_t appearance() const override;
  void onLink(bool connected) override;

  void onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo& connInfo, uint16_t subValue) override;

 private:
  void push(const GamepadFrame& frame);
  void notify(HidCapability capability, const uint8_t* data, size_t len);

  std::string manufacturer_;
  IHidProfileRepository& profiles_;
  HidCapabilitySet profile_;
  NimBLEHIDDevice* hid_ = nullptr;
  NimBLECharacteristic* reports_[kHidCapabilityCount] = {};
  HidCapabilitySet subscribed_;
  bool active_ = false;
  GamepadFrame last_;
};

}  // namespace tama

#endif
