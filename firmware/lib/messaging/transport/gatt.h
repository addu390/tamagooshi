#pragma once
#if defined(TAMA_PROTO_GATT)

#include <NimBLEDevice.h>

#include <cstdint>
#include <string>

#include "ble/ble.h"
#include "transport.h"

namespace tama {

namespace gatt {
inline constexpr char kService[] = "9e7b0001-8c9a-4f2b-8b7a-1e2d3c4b5a6f";
inline constexpr char kInbound[] = "9e7b0002-8c9a-4f2b-8b7a-1e2d3c4b5a6f";
inline constexpr char kOutbound[] = "9e7b0003-8c9a-4f2b-8b7a-1e2d3c4b5a6f";
inline constexpr char kInfo[] = "9e7b0004-8c9a-4f2b-8b7a-1e2d3c4b5a6f";
}  // namespace gatt

class HubEndpoint : public ITransport, public IBleService, public NimBLECharacteristicCallbacks {
 public:
  HubEndpoint(BleBearer& bearer, std::string brand, std::string fwVersion);

  void begin() override {}
  void loop() override {}
  void publish(const std::string& topic, const std::string& payload, uint8_t qos,
               bool retain) override;
  bool connected() const override;
  void onMessage(MessageHandler handler) override;
  void onConnection(ConnectionHandler handler) override;

  void setup(BleBearer& bearer, NimBLEServer* nim) override;
  const char* serviceUuid() const override { return gatt::kService; }
  bool advertiseUuid() const override { return true; }
  void onLink(bool connected) override;

  void onWrite(NimBLECharacteristic* chr) override;
  void onSubscribe(NimBLECharacteristic* chr, ble_gap_conn_desc* desc, uint16_t subValue) override;

 private:
  void ingest(const uint8_t* data, size_t len);
  uint16_t chunkSize() const;

  BleBearer& bearer_;
  std::string brand_;
  std::string fw_;
  std::string id_;

  NimBLECharacteristic* inbound_ = nullptr;
  NimBLECharacteristic* outbound_ = nullptr;
  bool subscribed_ = false;

  std::string rx_;
  MessageHandler messageHandler_;
  ConnectionHandler connectionHandler_;
};

}  // namespace tama

#endif
