#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <string>

#include "model.h"
#include "transport/noise.h"

namespace tama {

class MuseCloud {
 public:
  MuseCloud(IAccountRepository& accounts, std::string node, std::string sdkToken);

  muse::Resolution resolve(muse::Target& target);

 private:
  enum class Reply { Ok, Auth, Failed };

  Reply fetch(const Account& account, muse::Vm& vm);
  Reply refresh(Account& account);
  Reply call(const Account& account, const char* path, const std::string& auth,
             const std::string* body, std::string& out);

  IAccountRepository& accounts_;
  std::string node_;
  std::string sdkToken_;
  bool reported_ = false;
};

}  // namespace tama

#endif
