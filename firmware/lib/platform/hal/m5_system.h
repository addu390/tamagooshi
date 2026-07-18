#pragma once

#include <M5Unified.h>
#include <esp_system.h>
#include <sys/time.h>

#include <ctime>

#include "system.h"

namespace tama {

class M5SystemControl : public ISystemControl {
 public:
  void reboot() override { esp_restart(); }
  void setBrightness(uint8_t level) override { M5.Display.setBrightness(level); }

  void setClock(int64_t epochSecs) override {
    if (epochSecs <= 0) return;
    const timeval tv{static_cast<time_t>(epochSecs), 0};
    settimeofday(&tv, nullptr);

    std::tm utc{};
    const time_t t = static_cast<time_t>(epochSecs);
    gmtime_r(&t, &utc);
    m5::rtc_datetime_t dt;
    dt.date.year = utc.tm_year + 1900;
    dt.date.month = utc.tm_mon + 1;
    dt.date.date = utc.tm_mday;
    dt.time.hours = utc.tm_hour;
    dt.time.minutes = utc.tm_min;
    dt.time.seconds = utc.tm_sec;
    M5.Rtc.setDateTime(dt);
  }
};

}  // namespace tama
