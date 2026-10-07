#include "ble/setup.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp32-hal-log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cstring>
#include <utility>

namespace tama {

namespace {

constexpr const char* kTag = "setup";
constexpr const char* kService = "7fdd3d1c-38ea-46cf-8b46-314ecf5f240c";
constexpr const char* kRx = "4d593029-28a2-4a6e-a1f0-3c2d5e8f9b01";
constexpr const char* kTx = "d75dc4ca-7b2b-4e9c-8f0a-1d2e3f4a5b6c";

constexpr uint8_t kChunkMagic = 0xFE;
constexpr size_t kChunkHeader = 3;
constexpr size_t kMaxNotify = 160;
constexpr size_t kMaxCommand = 8192;
constexpr uint32_t kChunkGapMs = 50;

bool sensitive(const std::string& action) {
  return action == "provision" || action == "provision_v2" || action == "wifi_scan" ||
         action == "ota" || action == "device.ota" || action == "unpair" ||
         action == "set_wifi" || action == "set_auth";
}

}  // namespace

SetupEndpoint::SetupEndpoint(BleBearer& bearer, muse::Device device, std::string name,
                             std::string sdkToken)
    : bearer_(bearer),
      pairing_(std::move(device)),
      name_(std::move(name)),
      sdkToken_(std::move(sdkToken)) {}

void SetupEndpoint::setup(BleBearer&, NimBLEServer* nim) {
  NimBLEService* svc = nim->createService(kService);
  rx_ = svc->createCharacteristic(kRx, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rx_->setCallbacks(this);
  tx_ = svc->createCharacteristic(kTx, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  tx_->setCallbacks(this);
}

const char* SetupEndpoint::serviceUuid() const { return kService; }

void SetupEndpoint::open(bool paired) {
  open_ = true;
  provisioning_ = false;
  pairing_.reset();
  if (!bearer_.enabled()) bearer_.setEnabled(true);
  bearer_.claim(*this, name_, std::string("\xFF\xFF", 2) + (paired ? '\x01' : '\x00'));
}

void SetupEndpoint::close() {
  if (!open_) return;
  open_ = false;
  provisioning_ = false;
  pairing_.reset();
  bearer_.release();
}

void SetupEndpoint::onLink(bool connected) {
  subscribed_ = false;
  std::lock_guard<std::mutex> lock(mutex_);
  commands_.clear();
  assembling_.clear();
  expected_ = 0;
  next_ = 0;
  if (!connected) reset_ = true;
}

void SetupEndpoint::onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo&, uint16_t subValue) {
  if (chr != tx_) return;
  subscribed_ = subValue != 0;
}

void SetupEndpoint::onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) {
  if (chr == rx_) ingest(chr->getValue());
}

void SetupEndpoint::ingest(const std::string& chunk) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto* b = reinterpret_cast<const uint8_t*>(chunk.data());
  if (chunk.size() < kChunkHeader || b[0] != kChunkMagic) {
    commands_.push_back(chunk);
    return;
  }
  const uint8_t index = b[1];
  const uint8_t total = b[2];
  if (total == 0) {
    assembling_.clear();
    return;
  }
  if (index == 0 || total != expected_) {
    assembling_.clear();
    expected_ = total;
    next_ = 0;
  }
  if (index != next_ || index >= total ||
      assembling_.size() + chunk.size() - kChunkHeader > kMaxCommand) {
    assembling_.clear();
    expected_ = 0;
    next_ = 0;
    return;
  }
  assembling_.append(chunk, kChunkHeader, std::string::npos);
  if (++next_ < total) return;
  commands_.push_back(std::move(assembling_));
  assembling_.clear();
  expected_ = 0;
  next_ = 0;
}

void SetupEndpoint::loop() {
  std::deque<std::string> commands;
  bool reset = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    commands.swap(commands_);
    std::swap(reset, reset_);
  }
  if (reset) {
    pairing_.reset();
    provisioning_ = false;
  }
  if (!open_) return;
  if (pairing_.expired()) abandon("pairing_confirm_timeout");
  for (const std::string& command : commands) handle(command);
}

void SetupEndpoint::handle(const std::string& command) {
  const std::string action = muse::decodeAction(command);
  if (action == "pairing_client_hello") {
    muse::Hello hello;
    muse::Ready ready;
    if (!muse::decodeHello(command, hello) || !pairing_.hello(hello, ready)) {
      ESP_LOGW(kTag, "hello rejected");
      return plain("error_pairing_invalid_hello");
    }
    return send(muse::encodeReady(pairing_.device(), ready));
  }
  if (action == "pairing_encrypted") return handleSealed(command);
  if (pairing_.keyed()) return;
  if (sensitive(action)) return plain("error_encryption_required");
  if (action == "get_device_info") return send(muse::encodeDeviceInfo(pairing_.device()));
  plain("error_unknown_action");
}

void SetupEndpoint::handleSealed(const std::string& command) {
  muse::Sealed envelope;
  std::string inner;
  if (!muse::decodeSealed(command, envelope) || !pairing_.open(envelope, inner)) {
    ESP_LOGW(kTag, "sealed decode failed");
    pairing_.reset();
    plain("error_pairing_decrypt");
    return bearer_.disconnect();
  }
  const std::string action = muse::decodeAction(inner);

  if (action == "pairing_client_finished") {
    if (!muse::isClientFinished(inner) || !pairing_.finish()) {
      pairing_.reset();
      plain("error_pairing_decrypt");
      return bearer_.disconnect();
    }
    status("confirm_required");
    if (confirmHandler_) confirmHandler_();
    return;
  }
  if (sensitive(action) && pairing_.phase() != Pairing::Phase::Ready)
    return status("error_pairing_confirm_required");
  if (action == "wifi_scan") {
    return sealed(muse::encodeScan(scan_ ? scan_() : std::vector<std::string>{}));
  }
  if (action == "get_device_info") return send(muse::encodeDeviceInfo(pairing_.device()));
  if (action != "provision_v2") return status("error_unknown_action");

  muse::Provision provision;
  if (!muse::decodeProvision(inner, provision)) return status("error_missing_credentials");
  if (provisioning_) return status("error_operation_in_progress");
  provisioning_ = true;
  if (provisionHandler_) provisionHandler_(provision);
}

void SetupEndpoint::confirm(bool allow) {
  if (pairing_.phase() != Pairing::Phase::Confirming) return;
  if (!allow) return abandon("pairing_confirm_timeout");
  if (pairing_.confirm()) sealed(muse::encodeStatus("pairing_confirmed", sdkToken_));
}

void SetupEndpoint::status(const char* status) { sealed(muse::encodeStatus(status)); }

void SetupEndpoint::abandon(const char* status) {
  ESP_LOGW(kTag, "abandon: %s", status);
  this->status(status);
  pairing_.reset();
  provisioning_ = false;
  bearer_.disconnect();
}

void SetupEndpoint::plain(const char* status) {
  if (!subscribed_ || !tx_) return;
  tx_->setValue(reinterpret_cast<const uint8_t*>(status), std::strlen(status));
  tx_->notify(bearer_.connHandle());
}

void SetupEndpoint::sealed(const std::string& json) {
  muse::Sealed envelope;
  if (pairing_.seal(json, envelope)) send(muse::encodeSealed(envelope));
}

void SetupEndpoint::send(const std::string& message) {
  if (!subscribed_ || !tx_) {
    ESP_LOGW(kTag, "tx dropped %u bytes: not subscribed", static_cast<unsigned>(message.size()));
    return;
  }
  const size_t notify = std::min<size_t>(bearer_.peerMtu() - 3, kMaxNotify);
  if (notify <= kChunkHeader) return;
  const size_t usable = notify - kChunkHeader;
  const size_t total = std::max<size_t>(1, (message.size() + usable - 1) / usable);
  if (total > 255) return;
  uint8_t buf[kMaxNotify];
  for (size_t i = 0; i < total; ++i) {
    const size_t off = i * usable;
    const size_t len = std::min(usable, message.size() - off);
    buf[0] = kChunkMagic;
    buf[1] = static_cast<uint8_t>(i);
    buf[2] = static_cast<uint8_t>(total);
    std::memcpy(buf + kChunkHeader, message.data() + off, len);
    tx_->setValue(buf, kChunkHeader + len);
    if (!tx_->notify(bearer_.connHandle())) {
      ESP_LOGW(kTag, "notify failed at chunk %u", static_cast<unsigned>(i));
      return;
    }
    vTaskDelay(pdMS_TO_TICKS(kChunkGapMs));
  }
}

}  // namespace tama

#endif
