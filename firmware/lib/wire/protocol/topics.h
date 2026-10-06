#pragma once

#include <string>

namespace tama::topics {

inline std::string acks(const std::string& id) { return "devices/" + id + "/acks"; }
inline std::string status(const std::string& id) { return "devices/" + id + "/status"; }
inline std::string hello(const std::string& id) { return "devices/" + id + "/hello"; }
inline std::string input(const std::string& id) { return "devices/" + id + "/input"; }
inline std::string sensor(const std::string& id) { return "devices/" + id + "/sensor"; }

}  // namespace tama::topics
