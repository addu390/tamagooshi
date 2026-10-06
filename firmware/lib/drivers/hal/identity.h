#pragma once

#include <esp_mac.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace tama::identity {

inline std::string suffix() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[5];
  std::snprintf(buf, sizeof(buf), "%02X%02X", mac[4], mac[5]);
  return std::string(buf);
}

inline std::string deviceId() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[13];
  std::snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
                mac[5]);
  return std::string(buf);
}

inline std::string mac() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3],
                mac[4], mac[5]);
  return std::string(buf);
}

inline std::string node() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[16];
  std::snprintf(buf, sizeof(buf), "homelink-%02x%02x%02x", mac[3], mac[4], mac[5]);
  return std::string(buf);
}

inline std::string gadget() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  std::snprintf(buf, sizeof(buf), "MuseGadget-%02X%02X%02X", mac[3], mac[4], mac[5]);
  return std::string(buf);
}

inline std::string deviceName(const std::string& brand) {
  std::string slug = brand.empty() ? "tama" : brand;
  if (slug.size() > 16) slug.resize(16);
  return slug + "-" + suffix();
}

}  // namespace tama::identity
