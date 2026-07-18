#include "ble/ble.h"
#if defined(TAMA_ENABLE_BLE)

#include <Preferences.h>
#include <esp_random.h>

#include <utility>

#include "hal/identity.h"

namespace tama {

namespace {
constexpr uint16_t kRequestedMtu = 247;
constexpr size_t kMaxNameLen = 29;
constexpr char kNamespace[] = "blenet";
}  // namespace

BleBearer::BleBearer(std::string brand, std::string fwVersion)
    : brand_(std::move(brand)), fw_(std::move(fwVersion)) {}

void BleBearer::add(IBleService& service) { services_.push_back(&service); }

void BleBearer::begin() {
  id_ = identity::deviceId();
  // The "Claude-" prefix is required by the claude-desktop-buddy protocol: the
  // desktop picker only lists devices whose advertised name starts with "Claude".
  name_ = "Claude-" + (brand_.empty() ? std::string("tama") : brand_) + "-" + identity::suffix();
  if (name_.size() > kMaxNameLen) name_.resize(kMaxNameLen);
  passkey_ = 100000 + (esp_random() % 900000);
  paired_ = false;

  Preferences prefs;
  if (prefs.begin(kNamespace, true)) {
    enabled_ = prefs.getUChar("on", 1) != 0;
    prefs.end();
  }

  NimBLEDevice::init(name_);
  NimBLEDevice::setMTU(kRequestedMtu);
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
  NimBLEDevice::setSecurityPasskey(passkey_);

  server_ = NimBLEDevice::createServer();
  server_->setCallbacks(this);

  for (IBleService* service : services_) service->setup(*this, server_);

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  for (IBleService* service : services_) {
    if (service->advertiseUuid()) adv->addServiceUUID(service->serviceUuid());
  }
  adv->setName(name_);
  adv->setScanResponse(true);
  if (enabled_) NimBLEDevice::startAdvertising();
  started_ = true;
}

void BleBearer::setEnabled(bool on) {
  if (on == enabled_) return;
  enabled_ = on;

  Preferences prefs;
  if (prefs.begin(kNamespace, false)) {
    prefs.putUChar("on", on ? 1 : 0);
    prefs.end();
  }

  if (on) {
    NimBLEDevice::startAdvertising();
  } else {
    if (central_) server_->disconnect(connHandle_);
    NimBLEDevice::stopAdvertising();
  }
}

std::string BleBearer::peer() const { return central_ ? "paired device" : std::string{}; }

uint16_t BleBearer::peerMtu() const {
  uint16_t mtu = server_ ? server_->getPeerMTU(connHandle_) : 0;
  if (mtu < 23) mtu = 23;
  return mtu;
}

void BleBearer::unpair() {
  NimBLEDevice::deleteAllBonds();
  paired_ = false;
  if (central_) NimBLEDevice::getServer()->disconnect(connHandle_);
}

void BleBearer::onConnect(NimBLEServer*, ble_gap_conn_desc* desc) {
  connHandle_ = desc->conn_handle;
  central_ = true;
  for (IBleService* service : services_) service->onLink(true);
}

void BleBearer::onDisconnect(NimBLEServer*) {
  central_ = false;
  paired_ = false;
  for (IBleService* service : services_) service->onLink(false);
  if (enabled_) NimBLEDevice::startAdvertising();
}

void BleBearer::onAuthenticationComplete(ble_gap_conn_desc* desc) {
  paired_ = desc->sec_state.encrypted;
}

}  // namespace tama

#endif
