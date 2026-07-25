#pragma once

#include "ir.h"

namespace tama {

class NvsIrCodeRepository : public IIrCodeRepository {
 public:
  int load(IrButton* out, int max) override;
  void save(const IrButton* buttons, int count) override;
};

}  // namespace tama
