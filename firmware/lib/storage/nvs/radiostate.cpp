#include "nvs/radiostate.h"

#ifdef ARDUINO

#include "nvs/scope.h"

namespace tama {

namespace {
constexpr char kEnabledKey[] = "on";
}  // namespace

std::optional<bool> NvsRadioStateRepository::enabled() const {
  nvs::Scope scope(space_, nvs::Access::Read);
  if (!scope || !scope->isKey(kEnabledKey)) return std::nullopt;
  return scope->getUChar(kEnabledKey, 0) != 0;
}

void NvsRadioStateRepository::setEnabled(bool on) {
  nvs::Scope scope(space_, nvs::Access::Write);
  if (!scope) return;
  scope->putUChar(kEnabledKey, on ? 1 : 0);
}

}  // namespace tama

#endif  // ARDUINO
