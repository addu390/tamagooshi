#pragma once

#if defined(TAMA_ENABLE_WIFI)

#include "wifi/credentials.h"

namespace tama {

class NvsNetworkRepository : public INetworkRepository {
 public:
  std::vector<WifiCredentials> all() const override;
  void remember(const WifiCredentials& creds) override;
  void forget(const std::string& ssid) override;
  void clear() override;

 private:
  void write(const std::vector<WifiCredentials>& list);
};

}  // namespace tama

#endif  // TAMA_ENABLE_WIFI
