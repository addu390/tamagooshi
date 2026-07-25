#include "wifi/wifi.h"
#if defined(TAMA_ENABLE_WIFI)

#include <WiFi.h>

namespace tama {

namespace {
constexpr uint32_t kRetryIntervalMs = 5000;
constexpr int kFailuresBeforeAdvance = 3;
}  // namespace

WifiBearer::WifiBearer(INetworkRepository& networks, IProvisioner& provisioner,
                       IRadioStateRepository& state)
    : networks_(networks), provisioner_(provisioner), state_(state) {}

void WifiBearer::begin() {
  provisioner_.onCredentials([this](const WifiCredentials& creds) {
    networks_.remember(creds);
    provisioner_.stop();
    provisioning_ = false;
    connect(creds);
  });

  enabled_ = loadEnabled();
  if (enabled_) start();
}

void WifiBearer::start() {
  const std::vector<WifiCredentials> known = networks_.all();
  if (known.empty()) {
    provisioning_ = true;
    provisioner_.begin();
    return;
  }
  WifiCredentials target;
  if (!credentialsFor(activeSsid_, target)) target = known.front();
  connect(target);
}

void WifiBearer::stop() {
  provisioning_ = false;
  provisioner_.stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  activeSsid_.clear();
}

void WifiBearer::connect(const WifiCredentials& creds) {
  activeSsid_ = creds.ssid;
  failures_ = 0;
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(creds.ssid.c_str(), creds.password.c_str());
  lastAttemptMs_ = millis();
}

void WifiBearer::loop() {
  if (!enabled_) return;
  if (provisioning_) {
    provisioner_.loop();
    return;
  }
  if (online()) {
    failures_ = 0;
    return;
  }
  const uint32_t now = millis();
  if (now - lastAttemptMs_ < kRetryIntervalMs) return;
  lastAttemptMs_ = now;
  if (++failures_ >= kFailuresBeforeAdvance) advance();
  WifiCredentials target;
  if (credentialsFor(activeSsid_, target)) {
    WiFi.disconnect();
    WiFi.begin(target.ssid.c_str(), target.password.c_str());
  }
}

void WifiBearer::advance() {
  const std::vector<WifiCredentials> known = networks_.all();
  if (known.size() < 2) return;
  size_t idx = 0;
  for (size_t i = 0; i < known.size(); ++i) {
    if (known[i].ssid == activeSsid_) idx = i;
  }
  activeSsid_ = known[(idx + 1) % known.size()].ssid;
  failures_ = 0;
}

bool WifiBearer::online() const { return WiFi.status() == WL_CONNECTED; }

bool WifiBearer::connected() const { return online(); }

std::string WifiBearer::peer() const { return online() ? WiFi.SSID().c_str() : std::string{}; }

void WifiBearer::setEnabled(bool on) {
  if (on == enabled_) return;
  enabled_ = on;
  saveEnabled(on);
  if (on)
    start();
  else
    stop();
}

std::vector<KnownNetwork> WifiBearer::known() const {
  std::vector<KnownNetwork> out;
  for (const WifiCredentials& c : networks_.all()) out.push_back({c.ssid, c.ssid == activeSsid_});
  return out;
}

void WifiBearer::select(const std::string& ssid) {
  WifiCredentials target;
  if (!credentialsFor(ssid, target)) return;
  if (!enabled_) {
    enabled_ = true;
    saveEnabled(true);
  }
  provisioning_ = false;
  provisioner_.stop();
  connect(target);
}

void WifiBearer::forget(const std::string& ssid) {
  networks_.forget(ssid);
  if (ssid != activeSsid_) return;
  WifiCredentials next;
  const std::vector<WifiCredentials> known = networks_.all();
  if (!known.empty()) {
    next = known.front();
    connect(next);
  } else {
    WiFi.disconnect(true);
    activeSsid_.clear();
  }
}

void WifiBearer::provision() {
  if (!enabled_) {
    enabled_ = true;
    saveEnabled(true);
  }
  provisioning_ = true;
  provisioner_.begin();
}

bool WifiBearer::credentialsFor(const std::string& ssid, WifiCredentials& out) const {
  if (ssid.empty()) return false;
  for (const WifiCredentials& c : networks_.all()) {
    if (c.ssid == ssid) {
      out = c;
      return true;
    }
  }
  return false;
}

bool WifiBearer::loadEnabled() const {
  return state_.enabled().value_or(!networks_.all().empty());
}

void WifiBearer::saveEnabled(bool on) { state_.setEnabled(on); }

}  // namespace tama

#endif
