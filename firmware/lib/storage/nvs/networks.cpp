#include "nvs/networks.h"

#if defined(TAMA_ENABLE_WIFI) && defined(ARDUINO)

#include "nvs/scope.h"

namespace tama {

namespace {
std::string ssidKey(size_t i) { return "ssid" + std::to_string(i); }
std::string passKey(size_t i) { return "pass" + std::to_string(i); }
}  // namespace

std::vector<WifiCredentials> NvsNetworkRepository::all() const {
  std::vector<WifiCredentials> list;
  nvs::Scope scope(nvs::kNetworks, nvs::Access::Read);
  if (!scope) return list;
  const size_t count = scope->getUChar("count", 0);
  for (size_t i = 0; i < count && i < kMaxKnownNetworks; ++i) {
    WifiCredentials c;
    c.ssid = scope->getString(ssidKey(i).c_str(), "").c_str();
    c.password = scope->getString(passKey(i).c_str(), "").c_str();
    if (c.valid()) list.push_back(std::move(c));
  }
  return list;
}

void NvsNetworkRepository::remember(const WifiCredentials& creds) {
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

void NvsNetworkRepository::forget(const std::string& ssid) {
  std::vector<WifiCredentials> list = all();
  for (auto it = list.begin(); it != list.end(); ++it) {
    if (it->ssid == ssid) {
      list.erase(it);
      break;
    }
  }
  write(list);
}

void NvsNetworkRepository::clear() { write({}); }

void NvsNetworkRepository::write(const std::vector<WifiCredentials>& list) {
  nvs::Scope scope(nvs::kNetworks, nvs::Access::Write);
  if (!scope) return;
  scope->putUChar("count", static_cast<uint8_t>(list.size()));
  for (size_t i = 0; i < list.size(); ++i) {
    scope->putString(ssidKey(i).c_str(), list[i].ssid.c_str());
    scope->putString(passKey(i).c_str(), list[i].password.c_str());
  }
}

}  // namespace tama

#endif
