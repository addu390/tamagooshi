#pragma once

#include "hid.h"

namespace tama {

class NvsHidProfileRepository : public IHidProfileRepository {
 public:
  std::optional<HidCapabilitySet> load() override;
  void save(HidCapabilitySet profile) override;
};

}  // namespace tama
