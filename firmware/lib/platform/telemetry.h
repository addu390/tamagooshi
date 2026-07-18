#pragma once

#include <cstdint>

namespace tama {

struct Telemetry {
  int batt_pct = 0;
  int mV = 0;
  int mA = 0;
  bool usb = false;
  uint32_t up_secs = 0;
  uint32_t heap = 0;
};

class ITelemetry {
 public:
  virtual ~ITelemetry() = default;
  virtual Telemetry read() = 0;
};

class NullTelemetry : public ITelemetry {
 public:
  Telemetry read() override { return {}; }
};

}  // namespace tama
