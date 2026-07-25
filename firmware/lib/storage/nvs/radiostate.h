#pragma once

#include "radio.h"

namespace tama {

class NvsRadioStateRepository : public IRadioStateRepository {
 public:
  explicit NvsRadioStateRepository(const char* space) : space_(space) {}

  std::optional<bool> enabled() const override;
  void setEnabled(bool on) override;

 private:
  const char* space_;
};

}  // namespace tama
