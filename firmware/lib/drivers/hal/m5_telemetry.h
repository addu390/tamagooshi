#pragma once

#include <M5Unified.h>
#include <esp_system.h>
#include <esp_timer.h>

#include "telemetry.h"

namespace tama {

class M5Telemetry : public ITelemetry {
 public:
  Telemetry read() override {
    Telemetry t;
    t.batt_pct = M5.Power.getBatteryLevel();
    t.mV = M5.Power.getBatteryVoltage();
    t.mA = M5.Power.getBatteryCurrent();
    t.usb = M5.Power.isCharging() == m5::Power_Class::is_charging;
    t.up_secs = static_cast<uint32_t>(esp_timer_get_time() / 1000000);
    t.heap = esp_get_free_heap_size();
    return t;
  }
};

}  // namespace tama
