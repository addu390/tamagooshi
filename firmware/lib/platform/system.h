#pragma once

#include <cstdint>

namespace tama {

class ISystemControl {
 public:
  virtual ~ISystemControl() = default;
  virtual void reboot() = 0;
  virtual void setBrightness(uint8_t level) = 0;
  virtual void setClock(int64_t epochSecs) = 0;
};

}  // namespace tama
