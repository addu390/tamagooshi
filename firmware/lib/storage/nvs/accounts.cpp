#include "nvs/accounts.h"

#if defined(TAMA_AGENT_MUSE) && defined(ARDUINO)

#include "nvs/scope.h"

namespace tama {

namespace {
constexpr char kAccessKey[] = "access";
constexpr char kRefreshKey[] = "refresh";
constexpr char kApiKey[] = "api";
constexpr char kNoiseKey[] = "noise";
}  // namespace

std::optional<Account> NvsAccountRepository::load() const {
  nvs::Scope scope(nvs::kAccounts, nvs::Access::Read);
  if (!scope) return std::nullopt;
  Account account;
  account.access = scope->getString(kAccessKey, "").c_str();
  account.refresh = scope->getString(kRefreshKey, "").c_str();
  account.api = scope->getString(kApiKey, "").c_str();
  account.noise = scope->getString(kNoiseKey, "").c_str();
  if (!account.valid()) return std::nullopt;
  return account;
}

bool NvsAccountRepository::save(const Account& account) {
  nvs::Scope scope(nvs::kAccounts, nvs::Access::Write);
  if (!scope) return false;
  return scope->putString(kAccessKey, account.access.c_str()) == account.access.size() &&
         scope->putString(kRefreshKey, account.refresh.c_str()) == account.refresh.size() &&
         scope->putString(kApiKey, account.api.c_str()) == account.api.size() &&
         scope->putString(kNoiseKey, account.noise.c_str()) == account.noise.size();
}

void NvsAccountRepository::clear() {
  nvs::Scope scope(nvs::kAccounts, nvs::Access::Write);
  if (scope) scope->clear();
}

}  // namespace tama

#endif
