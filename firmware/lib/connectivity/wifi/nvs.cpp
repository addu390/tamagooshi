#include "wifi/nvs.h"
#if defined(TAMA_ENABLE_WIFI)

#include <Preferences.h>

namespace tama {

namespace {
constexpr char kNamespace[] = "wifi";

std::string ssidKey(size_t i) { return "ssid" + std::to_string(i); }
std::string passKey(size_t i) { return "pass" + std::to_string(i); }
}  // namespace

std::vector<WifiCredentials> NvsCredentialStore::all() const {
  Preferences prefs;
  std::vector<WifiCredentials> list;
  if (!prefs.begin(kNamespace, true)) return list;
  const size_t count = prefs.getUChar("count", 0);
  for (size_t i = 0; i < count && i < kMaxKnownNetworks; ++i) {
    WifiCredentials c;
    c.ssid = prefs.getString(ssidKey(i).c_str(), "").c_str();
    c.password = prefs.getString(passKey(i).c_str(), "").c_str();
    if (c.valid()) list.push_back(std::move(c));
  }
  prefs.end();
  return list;
}

void NvsCredentialStore::remember(const WifiCredentials& creds) {
  if (!creds.valid()) return;
  std::vector<WifiCredentials> list = all();
  for (auto it = list.begin(); it != list.end(); ++it) {
    if (it->ssid == creds.ssid) {
      list.erase(it);
      break;
    }
  }
  list.insert(list.begin(), creds);
  if (list.size() > kMaxKnownNetworks) list.resize(kMaxKnownNetworks);
  write(list);
}

void NvsCredentialStore::forget(const std::string& ssid) {
  std::vector<WifiCredentials> list = all();
  for (auto it = list.begin(); it != list.end(); ++it) {
    if (it->ssid == ssid) {
      list.erase(it);
      break;
    }
  }
  write(list);
}

void NvsCredentialStore::clear() { write({}); }

void NvsCredentialStore::write(const std::vector<WifiCredentials>& list) {
  Preferences prefs;
  if (!prefs.begin(kNamespace, false)) return;
  prefs.putUChar("count", static_cast<uint8_t>(list.size()));
  for (size_t i = 0; i < list.size(); ++i) {
    prefs.putString(ssidKey(i).c_str(), list[i].ssid.c_str());
    prefs.putString(passKey(i).c_str(), list[i].password.c_str());
  }
  prefs.end();
}

}  // namespace tama

#endif
