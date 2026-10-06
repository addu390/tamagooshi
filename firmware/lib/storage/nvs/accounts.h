#pragma once

#if defined(TAMA_AGENT_MUSE)

#include "account.h"

namespace tama {

class NvsAccountRepository : public IAccountRepository {
 public:
  std::optional<Account> load() const override;
  bool save(const Account& account) override;
  void clear() override;
};

}  // namespace tama

#endif  // TAMA_AGENT_MUSE
