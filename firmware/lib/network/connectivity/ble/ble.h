#pragma once
#if defined(TAMA_ENABLE_BLE)

#include <NimBLEDevice.h>

#include <cstdint>
#include <string>
#include <vector>

#include "bearer.h"
#include "link.h"

namespace tama {

class BleBearer;

class IBleService {
 public:
  virtual ~IBleService() = default;
  virtual void setup(BleBearer& bearer, NimBLEServer* nim) = 0;
  virtual const char* serviceUuid() const = 0;
  virtual bool advertiseUuid() const { return false; }
  virtual uint16_t appearance() const { return 0; }
  virtual void onLink(bool connected) = 0;
};

class BleBearer : public IBearer, public ILink, public NimBLEServerCallbacks {
 public:
  BleBearer(std::string brand, std::string fwVersion);

  void add(IBleService& service);

  void begin() override;
  void loop() override {}
  bool online() const override { return started_; }

  uint16_t connHandle() const { return connHandle_; }
  bool central() const { return central_; }
  uint16_t peerMtu() const;

  bool available() const override { return true; }
  bool enabled() const override { return enabled_; }
  void setEnabled(bool on) override;
  bool connected() const override { return central_; }
  std::string peer() const override;

  std::string deviceName() const override { return name_; }
  std::string deviceId() const override { return id_; }
  uint32_t passkey() const override { return passkey_; }
  bool paired() const override { return paired_; }
  void unpair() override;

  void onConnect(NimBLEServer* server, NimBLEConnInfo& connInfo) override;
  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connInfo, int reason) override;
  void onAuthenticationComplete(NimBLEConnInfo& connInfo) override;

 private:
  std::string brand_;
  std::string fw_;
  std::string id_;
  std::string name_;
  uint32_t passkey_ = 0;
  bool paired_ = false;
  bool started_ = false;
  bool enabled_ = true;
  uint16_t connHandle_ = 0;
  bool central_ = false;

  NimBLEServer* server_ = nullptr;
  std::vector<IBleService*> services_;
};

}  // namespace tama

#endif
