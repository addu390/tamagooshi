#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <NimBLEDevice.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "ble/ble.h"
#include "ble/pairing.h"
#include "muse.h"

namespace tama {

class SetupEndpoint : public IBleService, public NimBLECharacteristicCallbacks {
 public:
  using ConfirmHandler = std::function<void()>;
  using ProvisionHandler = std::function<void(const muse::Provision&)>;
  using ScanFn = std::function<std::vector<std::string>()>;

  SetupEndpoint(BleBearer& bearer, muse::Device device, std::string name, std::string sdkToken);

  void open(bool paired);
  void close();
  bool isOpen() const { return open_; }
  void loop();

  void confirm(bool allow);
  void status(const char* status);
  Pairing::Phase phase() const { return pairing_.phase(); }
  const std::string& name() const { return name_; }

  void onConfirm(ConfirmHandler handler) { confirmHandler_ = std::move(handler); }
  void onProvision(ProvisionHandler handler) { provisionHandler_ = std::move(handler); }
  void onScan(ScanFn scan) { scan_ = std::move(scan); }

  void setup(BleBearer& bearer, NimBLEServer* nim) override;
  const char* serviceUuid() const override;
  bool exclusive() const override { return true; }
  void onLink(bool connected) override;

  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& info) override;
  void onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo& info, uint16_t subValue) override;

 private:
  void ingest(const std::string& chunk);
  void handle(const std::string& command);
  void handleSealed(const std::string& command);
  void abandon(const char* status);
  void plain(const char* status);
  void sealed(const std::string& json);
  void send(const std::string& message);

  BleBearer& bearer_;
  Pairing pairing_;
  std::string name_;
  std::string sdkToken_;
  NimBLECharacteristic* tx_ = nullptr;
  NimBLECharacteristic* rx_ = nullptr;
  std::atomic<bool> subscribed_{false};
  bool open_ = false;
  bool provisioning_ = false;

  std::mutex mutex_;
  std::deque<std::string> commands_;
  std::string assembling_;
  uint8_t expected_ = 0;
  uint8_t next_ = 0;
  bool reset_ = false;

  ConfirmHandler confirmHandler_;
  ProvisionHandler provisionHandler_;
  ScanFn scan_;
};

}  // namespace tama

#endif
