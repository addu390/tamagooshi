#include "nvs/hidprofile.h"

#ifdef ARDUINO

#include "nvs/scope.h"

namespace tama {

namespace {
constexpr char kProfileKey[] = "profile";
}  // namespace

std::optional<HidCapabilitySet> NvsHidProfileRepository::load() {
  nvs::Scope scope(nvs::kHidProfile, nvs::Access::Read);
  if (!scope || !scope->isKey(kProfileKey)) return std::nullopt;
  return HidCapabilitySet(scope->getUChar(kProfileKey, 0));
}

void NvsHidProfileRepository::save(HidCapabilitySet profile) {
  nvs::Scope scope(nvs::kHidProfile, nvs::Access::Write);
  if (!scope) return;
  scope->putUChar(kProfileKey, profile.bits());
}

}  // namespace tama

#endif  // ARDUINO
