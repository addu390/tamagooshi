#pragma once
#if defined(TAMA_ENABLE_BLE)

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <cstdint>
#include <string>

#include "ble/ble.h"
#include "gamepad.h"

namespace tama {

class GamepadEndpoint : public IGamepadLink,
                        public IBleService,
                        public NimBLECharacteristicCallbacks {
 public:
  explicit GamepadEndpoint(std::string manufacturer);

  void activate() override;
  void deactivate() override;
  bool ready() const override;
  void send(const GamepadFrame& frame) override;

  void setup(BleBearer& bearer, NimBLEServer* nim) override;
  const char* serviceUuid() const override;
  bool advertiseUuid() const override { return true; }
  uint16_t appearance() const override;
  void onLink(bool connected) override;

  void onSubscribe(NimBLECharacteristic* chr, ble_gap_conn_desc* desc, uint16_t subValue) override;

 private:
  void push(const GamepadFrame& frame);

  std::string manufacturer_;
  NimBLEHIDDevice* hid_ = nullptr;
  NimBLECharacteristic* input_ = nullptr;
  bool subscribed_ = false;
  bool active_ = false;
  GamepadFrame last_;
};

}  // namespace tama

#endif
