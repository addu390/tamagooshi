#pragma once

#include <M5Unified.h>

#include <utility>

#include "expression.h"
#include "system.h"
#include "telemetry.h"
#include "transport.h"

namespace tama {

class SimTransport : public ITransport {
 public:
  void begin() override {
    if (connectionHandler_) connectionHandler_(true);
  }
  void loop() override {}
  void publish(const std::string&, const std::string&, uint8_t, bool) override {}
  bool connected() const override { return true; }
  void onMessage(MessageHandler handler) override { messageHandler_ = std::move(handler); }
  void onConnection(ConnectionHandler handler) override { connectionHandler_ = std::move(handler); }

 private:
  MessageHandler messageHandler_;
  ConnectionHandler connectionHandler_;
};

class NullExpression : public IExpressionSink {
 public:
  void begin() override {}
  void apply(const ExpressionState&) override {}
  void play(const ExpressionCue&) override {}
};

class SimSystemControl : public ISystemControl {
 public:
  void reboot() override {}
  void setBrightness(uint8_t level) override { M5.Display.setBrightness(level); }
  void setClock(int64_t) override {}
};

class SimTelemetry : public ITelemetry {
 public:
  Telemetry read() override {
    Telemetry t;
    t.batt_pct = 100;
    return t;
  }
};

}  // namespace tama
