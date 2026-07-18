#pragma once

#include "model.h"

namespace tama {

class IBoardProfile {
 public:
  virtual ~IBoardProfile() = default;
  virtual const DeviceCapabilities& capabilities() const = 0;
};

}  // namespace tama
