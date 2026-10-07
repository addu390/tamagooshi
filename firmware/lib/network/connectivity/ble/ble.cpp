#include "ble/ble.h"
#if defined(TAMA_ENABLE_BLE)

#include <esp32-hal-log.h>
#include <esp_random.h>

#include <utility>

namespace tama {

namespace {
constexpr const char* kTag = "ble";
constexpr uint16_t kRequestedMtu = 247;
constexpr size_t kMaxNameLen = 29;
}  // namespace

BleBearer::BleBearer(std::string brand, std::string fwVersion, std::string deviceId,
                     IRadioStateRepository& state)
    : state_(state), brand_(std::move(brand)), fw_(std::move(fwVersion)) {
  id_ = std::move(deviceId);
}

void BleBearer::add(IBleService& service) { services_.push_back(&service); }

void BleBearer::begin() {
  const std::string suffix = id_.size() > 4 ? id_.substr(id_.size() - 4) : id_;
  name_ = (brand_.empty() ? std::string("tama") : brand_) + "-" + suffix;
  if (name_.size() > kMaxNameLen) name_.resize(kMaxNameLen);
  passkey_ = 100000 + (esp_random() % 900000);
  enabled_ = state_.enabled().value_or(false);
  start();
}

void BleBearer::claim(IBleService& owner, std::string name, std::string manufacturer) {
  const bool running = started_;
  stop();
  owner_ = &owner;
  ownerName_ = std::move(name);
  ownerData_ = std::move(manufacturer);
  if (running) start();
}

void BleBearer::release() {
  if (!owner_) return;
  const bool running = started_;
  stop();
  owner_ = nullptr;
  if (running) start();
}

void BleBearer::start() {
  paired_ = false;
  NimBLEDevice::init(owner_ ? ownerName_ : name_);
  NimBLEDevice::setMTU(kRequestedMtu);
  if (owner_) {
    uint8_t addr[6];
    esp_fill_random(addr, sizeof(addr));
    addr[5] |= 0xC0;
    NimBLEDevice::setOwnAddr(addr);
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
    NimBLEDevice::setSecurityAuth(false, false, false);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  } else {
    NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    NimBLEDevice::setSecurityPasskey(passkey_);
  }

  server_ = NimBLEDevice::createServer();
  server_->setCallbacks(this, false);
  for (IBleService* service : services_) {
    if (hosts(service)) service->setup(*this, server_);
  }

  started_ = true;
  advertise();
  ESP_LOGI(kTag, "started as %s%s", owner_ ? ownerName_.c_str() : name_.c_str(),
           owner_ ? " (exclusive)" : "");
}

void BleBearer::stop() {
  if (!started_) return;
  link(false);
  NimBLEDevice::deinit(true);
  server_ = nullptr;
  started_ = false;
}

bool BleBearer::hosts(const IBleService* service) const {
  return owner_ ? service == owner_ : !service->exclusive();
}

void BleBearer::link(bool connected) {
  central_ = connected;
  if (!connected) paired_ = false;
  for (IBleService* service : services_) {
    if (hosts(service)) service->onLink(connected);
  }
}

void BleBearer::disconnect() {
  if (central_) server_->disconnect(connHandle_);
}

void BleBearer::advertise() {
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  NimBLEDevice::stopAdvertising();
  adv->reset();
  if (owner_) {
    adv->addServiceUUID(owner_->serviceUuid());
    adv->setManufacturerData(ownerData_);
  } else {
    for (IBleService* service : services_) {
      if (!hosts(service)) continue;
      if (service->advertiseUuid()) adv->addServiceUUID(service->serviceUuid());
      if (const uint16_t look = service->appearance()) adv->setAppearance(look);
    }
  }
  adv->enableScanResponse(true);
  adv->setName(owner_ ? ownerName_ : name_);
  if (enabled_ && !central_) NimBLEDevice::startAdvertising();
}

void BleBearer::setEnabled(bool on) {
  if (on == enabled_) return;
  enabled_ = on;
  state_.setEnabled(on);

  if (on) {
    NimBLEDevice::startAdvertising();
  } else {
    disconnect();
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

void BleBearer::onConnect(NimBLEServer*, NimBLEConnInfo& connInfo) {
  ESP_LOGI(kTag, "connected %s", connInfo.getAddress().toString().c_str());
  connHandle_ = connInfo.getConnHandle();
  link(true);
}

void BleBearer::onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) {
  ESP_LOGI(kTag, "disconnected reason=0x%x", reason);
  link(false);
  if (enabled_) NimBLEDevice::startAdvertising();
}

void BleBearer::onAuthenticationComplete(NimBLEConnInfo& connInfo) {
  paired_ = connInfo.isEncrypted();
}

}  // namespace tama

#endif
