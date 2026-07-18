#pragma once
#if defined(TAMA_ENABLE_WIFI)

#include <DNSServer.h>
#include <WebServer.h>

#include <string>

#include "wifi/provisioner.h"

namespace tama {

class SoftApProvisioner : public IProvisioner {
 public:
  explicit SoftApProvisioner(std::string apName);

  void begin() override;
  void loop() override;
  bool active() const override { return active_; }
  void stop() override;
  void onCredentials(Handler handler) override { handler_ = std::move(handler); }
  std::string label() const override { return apName_; }

 private:
  void handleRoot();
  void handleSave();

  std::string apName_;
  DNSServer dns_;
  WebServer web_{80};
  Handler handler_;
  bool active_ = false;
};

}  // namespace tama

#endif
