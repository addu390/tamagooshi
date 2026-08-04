#pragma once

#ifdef ARDUINO

#include <Preferences.h>

namespace tama::nvs {

constexpr char kMetrics[] = "metrics";
constexpr char kHidProfile[] = "hid";
constexpr char kIrCodes[] = "remote";
constexpr char kNetworks[] = "wifi";
constexpr char kWifiRadio[] = "wifinet";
constexpr char kBleRadio[] = "blenet";
constexpr char kClocks[] = "clocks";

enum class Access { Read, Write };

class Scope {
 public:
  Scope(const char* space, Access access) : open_(prefs_.begin(space, access == Access::Read)) {}
  ~Scope() {
    if (open_) prefs_.end();
  }

  Scope(const Scope&) = delete;
  Scope& operator=(const Scope&) = delete;

  explicit operator bool() const { return open_; }
  Preferences* operator->() { return &prefs_; }

 private:
  Preferences prefs_;
  bool open_;
};

}  // namespace tama::nvs

#endif  // ARDUINO
