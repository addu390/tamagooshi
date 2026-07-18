#include "wifi/softap.h"
#if defined(TAMA_ENABLE_WIFI)

#include <WiFi.h>

#include <utility>

#include "portal.gen.h"

namespace tama {
namespace {

std::string render(const char* body, const std::string& ap, const std::string& ssids = "") {
  std::string page = std::string(portal::kHead) + body;
  for (const auto& [token, value] :
       {std::pair{"{{SSIDS}}", &ssids}, std::pair{"{{AP}}", &ap}}) {
    for (auto at = page.find(token); at != std::string::npos; at = page.find(token, at))
      page.replace(at, std::string(token).size(), *value);
  }
  return page;
}

}  // namespace

SoftApProvisioner::SoftApProvisioner(std::string apName) : apName_(std::move(apName)) {}

void SoftApProvisioner::begin() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName_.c_str());

  dns_.start(53, "*", WiFi.softAPIP());
  web_.on("/", [this] { handleRoot(); });
  web_.on("/save", HTTP_POST, [this] { handleSave(); });
  web_.onNotFound([this] { handleRoot(); });
  web_.begin();
  active_ = true;
}

void SoftApProvisioner::loop() {
  if (!active_) return;
  dns_.processNextRequest();
  web_.handleClient();
}

void SoftApProvisioner::stop() {
  if (!active_) return;
  web_.stop();
  dns_.stop();
  WiFi.softAPdisconnect(true);
  active_ = false;
}

void SoftApProvisioner::handleRoot() {
  std::string options;
  const int found = WiFi.scanNetworks();
  for (int i = 0; i < found; ++i) {
    const std::string ssid = WiFi.SSID(i).c_str();
    options += "<option value=\"" + ssid + "\">" + ssid + "</option>";
  }
  web_.send(200, "text/html", render(portal::kSetup, apName_, options).c_str());
}

void SoftApProvisioner::handleSave() {
  WifiCredentials creds;
  creds.ssid = web_.arg("ssid").c_str();
  creds.password = web_.arg("pass").c_str();
  web_.send(200, "text/html", render(portal::kSaved, apName_).c_str());
  if (handler_ && creds.valid()) handler_(creds);
}

}  // namespace tama

#endif
