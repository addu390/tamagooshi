#pragma once

#include <string>
#include <vector>

#include "radio.h"

namespace tama {

struct KnownNetwork {
  std::string ssid;
  bool active = false;
};

class IWifiControl : public IRadio {
 public:
  virtual std::vector<KnownNetwork> known() const = 0;
  virtual void select(const std::string& ssid) = 0;
  virtual void forget(const std::string& ssid) = 0;
  virtual void provision() = 0;
  virtual bool provisioning() const = 0;
  virtual std::string portal() const = 0;
};

}  // namespace tama
