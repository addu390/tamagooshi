#pragma once

#include "clock.h"

namespace tama {

class NvsClockRepository : public IClockRepository {
 public:
  std::optional<ClockSnapshot> load() override;
  void save(const ClockSnapshot& snapshot) override;
};

}  // namespace tama
