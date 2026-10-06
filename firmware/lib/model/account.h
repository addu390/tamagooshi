#pragma once

#include <optional>
#include <string>

namespace tama {

struct Account {
  std::string access;
  std::string refresh;
  std::string api;
  std::string noise;

  bool valid() const { return !access.empty() && !refresh.empty(); }
};

class IAccountRepository {
 public:
  virtual ~IAccountRepository() = default;
  virtual std::optional<Account> load() const = 0;
  virtual bool save(const Account& account) = 0;
  virtual void clear() = 0;
};

}  // namespace tama
