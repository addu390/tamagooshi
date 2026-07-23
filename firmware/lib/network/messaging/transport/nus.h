#pragma once
#if defined(TAMA_ENABLE_BLE)

#include <NimBLEDevice.h>

#include <cstdint>
#include <string>

#include "ble/ble.h"
#include "transport.h"

namespace tama {

namespace nus {
inline constexpr char kService[] = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
inline constexpr char kRx[] = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
inline constexpr char kTx[] = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
}  // namespace nus

class NusEndpoint : public IConnection,
                    public ILineSink,
                    public IBleService,
                    public NimBLECharacteristicCallbacks {
 public:
  explicit NusEndpoint(BleBearer& bearer);

  void begin() override {}
  void loop() override {}
  bool connected() const override;
  void onMessage(MessageHandler handler) override;
  void onConnection(ConnectionHandler handler) override;

  void send(const std::string& line) override;

  void setup(BleBearer& bearer, NimBLEServer* nim) override;
  const char* serviceUuid() const override { return nus::kService; }
  void onLink(bool connected) override;

  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo& connInfo) override;
  void onSubscribe(NimBLECharacteristic* chr, NimBLEConnInfo& connInfo, uint16_t subValue) override;

 private:
  void ingest(const uint8_t* data, size_t len);
  uint16_t chunkSize() const;

  BleBearer& bearer_;

  NimBLECharacteristic* rx_chr_ = nullptr;
  NimBLECharacteristic* tx_chr_ = nullptr;
  bool subscribed_ = false;

  std::string rx_;
  MessageHandler messageHandler_;
  ConnectionHandler connectionHandler_;
};

}  // namespace tama

#endif
