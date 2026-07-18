#include "transport/gatt.h"
#if defined(TAMA_PROTO_GATT)

#include <algorithm>
#include <utility>

#include "hal/identity.h"

namespace tama {

namespace {

void appendU16(std::string& out, uint16_t v) {
  out.push_back(static_cast<char>((v >> 8) & 0xFF));
  out.push_back(static_cast<char>(v & 0xFF));
}

void appendU32(std::string& out, uint32_t v) {
  out.push_back(static_cast<char>((v >> 24) & 0xFF));
  out.push_back(static_cast<char>((v >> 16) & 0xFF));
  out.push_back(static_cast<char>((v >> 8) & 0xFF));
  out.push_back(static_cast<char>(v & 0xFF));
}

std::string frame(const std::string& topic, const std::string& payload) {
  std::string msg;
  appendU16(msg, static_cast<uint16_t>(topic.size()));
  msg += topic;
  msg += payload;
  std::string wire;
  appendU32(wire, static_cast<uint32_t>(msg.size()));
  wire += msg;
  return wire;
}

}  // namespace

HubEndpoint::HubEndpoint(BleBearer& bearer, std::string brand, std::string fwVersion)
    : bearer_(bearer), brand_(std::move(brand)), fw_(std::move(fwVersion)) {}

void HubEndpoint::setup(BleBearer&, NimBLEServer* nim) {
  id_ = identity::deviceId();

  NimBLEService* svc = nim->createService(gatt::kService);
  inbound_ = svc->createCharacteristic(gatt::kInbound,
                                       NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
  inbound_->setCallbacks(this);
  outbound_ = svc->createCharacteristic(gatt::kOutbound, NIMBLE_PROPERTY::NOTIFY);
  outbound_->setCallbacks(this);
  NimBLECharacteristic* info =
      svc->createCharacteristic(gatt::kInfo, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC);
  info->setValue("{\"id\":\"" + id_ + "\",\"fw\":\"" + fw_ + "\",\"brand\":\"" + brand_ + "\"}");
  svc->start();
}

void HubEndpoint::publish(const std::string& topic, const std::string& payload, uint8_t, bool) {
  if (!subscribed_ || outbound_ == nullptr) return;
  const std::string wire = frame(topic, payload);
  const uint16_t chunk = chunkSize();
  for (size_t off = 0; off < wire.size(); off += chunk) {
    const size_t n = std::min<size_t>(chunk, wire.size() - off);
    outbound_->setValue(reinterpret_cast<const uint8_t*>(wire.data() + off), n);
    outbound_->notify(bearer_.connHandle());
  }
}

bool HubEndpoint::connected() const { return bearer_.central() && subscribed_; }

void HubEndpoint::onMessage(MessageHandler handler) { messageHandler_ = std::move(handler); }

void HubEndpoint::onConnection(ConnectionHandler handler) { connectionHandler_ = std::move(handler); }

uint16_t HubEndpoint::chunkSize() const { return static_cast<uint16_t>(bearer_.peerMtu() - 3); }

void HubEndpoint::onLink(bool connected) {
  if (connected) {
    rx_.clear();
    return;
  }
  subscribed_ = false;
  if (connectionHandler_) connectionHandler_(false);
}

void HubEndpoint::onWrite(NimBLECharacteristic* chr) {
  if (chr != inbound_) return;
  const std::string value = chr->getValue();
  ingest(reinterpret_cast<const uint8_t*>(value.data()), value.size());
}

void HubEndpoint::onSubscribe(NimBLECharacteristic* chr, ble_gap_conn_desc*, uint16_t subValue) {
  if (chr != outbound_) return;
  const bool wants = subValue != 0;
  if (wants == subscribed_) return;
  subscribed_ = wants;
  if (connectionHandler_) connectionHandler_(subscribed_);
}

void HubEndpoint::ingest(const uint8_t* data, size_t len) {
  rx_.append(reinterpret_cast<const char*>(data), len);
  while (rx_.size() >= 4) {
    const auto* b = reinterpret_cast<const uint8_t*>(rx_.data());
    const uint32_t total = (static_cast<uint32_t>(b[0]) << 24) | (static_cast<uint32_t>(b[1]) << 16) |
                           (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]);
    if (rx_.size() < 4 + total) break;
    if (total >= 2) {
      const uint16_t topicLen = (static_cast<uint16_t>(b[4]) << 8) | b[5];
      if (2 + topicLen <= total) {
        std::string topic(rx_.data() + 6, topicLen);
        std::string payload(rx_.data() + 6 + topicLen, total - 2 - topicLen);
        if (messageHandler_) messageHandler_(topic, payload);
      }
    }
    rx_.erase(0, 4 + total);
  }
}

}  // namespace tama

#endif
