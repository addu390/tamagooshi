#include "nus.h"
#if defined(TAMA_ENABLE_BLE)

#include <algorithm>
#include <utility>

namespace tama {

NusEndpoint::NusEndpoint(BleBearer& bearer) : bearer_(bearer) {}

void NusEndpoint::setup(BleBearer&, NimBLEServer* nim) {
  NimBLEService* svc = nim->createService(nus::kService);
  rx_chr_ = svc->createCharacteristic(
      nus::kRx, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_NR);
  rx_chr_->setCallbacks(this);
  tx_chr_ = svc->createCharacteristic(nus::kTx, NIMBLE_PROPERTY::NOTIFY);
  tx_chr_->setCallbacks(this);
  svc->start();
}

void NusEndpoint::send(const std::string& line) {
  if (!subscribed_ || tx_chr_ == nullptr) return;
  std::string wire = line;
  wire.push_back('\n');
  const uint16_t chunk = chunkSize();
  for (size_t off = 0; off < wire.size(); off += chunk) {
    const size_t n = std::min<size_t>(chunk, wire.size() - off);
    tx_chr_->setValue(reinterpret_cast<const uint8_t*>(wire.data() + off), n);
    tx_chr_->notify(bearer_.connHandle());
  }
}

bool NusEndpoint::connected() const { return bearer_.central() && subscribed_; }

void NusEndpoint::onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }

void NusEndpoint::onConnection(ConnectionHandler handler) { connectionHandler_ = std::move(handler); }

uint16_t NusEndpoint::chunkSize() const { return static_cast<uint16_t>(bearer_.peerMtu() - 3); }

void NusEndpoint::onLink(bool connected) {
  if (connected) {
    rx_.clear();
    return;
  }
  subscribed_ = false;
  if (connectionHandler_) connectionHandler_(false);
}

void NusEndpoint::onWrite(NimBLECharacteristic* chr) {
  if (chr != rx_chr_) return;
  const std::string value = chr->getValue();
  ingest(reinterpret_cast<const uint8_t*>(value.data()), value.size());
}

void NusEndpoint::onSubscribe(NimBLECharacteristic* chr, ble_gap_conn_desc*, uint16_t subValue) {
  if (chr != tx_chr_) return;
  const bool wants = subValue != 0;
  if (wants == subscribed_) return;
  subscribed_ = wants;
  if (connectionHandler_) connectionHandler_(subscribed_);
}

void NusEndpoint::ingest(const uint8_t* data, size_t len) {
  rx_.append(reinterpret_cast<const char*>(data), len);
  size_t nl;
  while ((nl = rx_.find('\n')) != std::string::npos) {
    std::string line = rx_.substr(0, nl);
    rx_.erase(0, nl + 1);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (!line.empty() && messageHandler_) messageHandler_("", line);
  }
}

}  // namespace tama

#endif
