#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace tama {

inline constexpr size_t kMaxKnownNetworks = 3;

struct WifiCredentials {
  std::string ssid;
  std::string password;

  bool valid() const { return !ssid.empty(); }
};

class ICredentialStore {
 public:
  virtual ~ICredentialStore() = default;
  virtual std::vector<WifiCredentials> all() const = 0;
  virtual void remember(const WifiCredentials& creds) = 0;
  virtual void forget(const std::string& ssid) = 0;
  virtual void clear() = 0;
};

}  // namespace tama
