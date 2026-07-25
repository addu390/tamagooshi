#pragma once

#include <utility>

#include "board.h"
#include "model.h"

namespace tama {

class StaticBoardProfile : public IBoardProfile {
 public:
  explicit StaticBoardProfile(DeviceCapabilities caps) : caps_(std::move(caps)) {}
  const DeviceCapabilities& capabilities() const override { return caps_; }

 private:
  DeviceCapabilities caps_;
};

}  // namespace tama
