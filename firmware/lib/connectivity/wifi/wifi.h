#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bearer.h"
#include "wifi/control.h"
#include "wifi/credentials.h"
#include "wifi/provisioner.h"

namespace tama {

class WifiBearer : public IBearer, public IWifiControl {
 public:
  WifiBearer(ICredentialStore& store, IProvisioner& provisioner);

  void begin() override;
  void loop() override;
  bool online() const override;

  bool available() const override { return true; }
  bool enabled() const override { return enabled_; }
  void setEnabled(bool on) override;
  bool connected() const override;
  std::string peer() const override;

  std::vector<KnownNetwork> known() const override;
  void select(const std::string& ssid) override;
  void forget(const std::string& ssid) override;
  void provision() override;
  bool provisioning() const override { return provisioning_; }
  std::string portal() const override { return provisioner_.label(); }

 private:
  void start();
  void stop();
  void connect(const WifiCredentials& creds);
  bool credentialsFor(const std::string& ssid, WifiCredentials& out) const;
  void advance();
  bool loadEnabled() const;
  void saveEnabled(bool on);

  ICredentialStore& store_;
  IProvisioner& provisioner_;
  std::string activeSsid_;
  bool enabled_ = false;
  bool provisioning_ = false;
  uint32_t lastAttemptMs_ = 0;
  int failures_ = 0;
};

}  // namespace tama
