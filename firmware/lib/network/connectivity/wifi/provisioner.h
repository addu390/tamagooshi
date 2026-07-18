#pragma once

#include <functional>
#include <string>

#include "wifi/credentials.h"

namespace tama {

class IProvisioner {
 public:
  using Handler = std::function<void(const WifiCredentials&)>;

  virtual ~IProvisioner() = default;
  virtual void begin() = 0;
  virtual void loop() = 0;
  virtual bool active() const = 0;
  virtual void stop() = 0;
  virtual void onCredentials(Handler handler) = 0;
  virtual std::string label() const = 0;
};

}  // namespace tama
