#pragma once

#include <string>

namespace tama::topics {

inline std::string metric(const std::string& id, const std::string& key) {
  return "devices/" + id + "/metrics/" + key;
}
inline std::string metricsWildcard(const std::string& id) {
  return "devices/" + id + "/metrics/+";
}
inline std::string branding(const std::string& id) { return "devices/" + id + "/branding"; }
inline std::string mood(const std::string& id) { return "devices/" + id + "/mood"; }
inline std::string pages(const std::string& id) { return "devices/" + id + "/pages"; }
inline std::string config(const std::string& id) { return "devices/" + id + "/config"; }
inline std::string acks(const std::string& id) { return "devices/" + id + "/acks"; }
inline std::string status(const std::string& id) { return "devices/" + id + "/status"; }
inline std::string hello(const std::string& id) { return "devices/" + id + "/hello"; }
inline std::string input(const std::string& id) { return "devices/" + id + "/input"; }
inline std::string sensor(const std::string& id) { return "devices/" + id + "/sensor"; }
inline std::string voice(const std::string& id) { return "devices/" + id + "/voice"; }
inline std::string expression(const std::string& id) { return "devices/" + id + "/expression"; }
inline std::string command(const std::string& id) { return "devices/" + id + "/command"; }
inline std::string deviceWildcard(const std::string& id) { return "devices/" + id + "/#"; }

inline const char* fleetWildcard() { return "fleet/all/#"; }

}  // namespace tama::topics
