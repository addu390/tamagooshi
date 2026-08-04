#include "nvs/clocks.h"

#ifdef ARDUINO

#include "nvs/scope.h"

namespace tama {

namespace {
constexpr char kEpochKey[] = "epoch";
constexpr char kTzKey[] = "tz";
}  // namespace

std::optional<ClockSnapshot> NvsClockRepository::load() {
  nvs::Scope scope(nvs::kClocks, nvs::Access::Read);
  if (!scope || !scope->isKey(kEpochKey)) return std::nullopt;

  ClockSnapshot snapshot;
  snapshot.epoch = scope->getLong64(kEpochKey, 0);
  snapshot.tz_offset_min = static_cast<int16_t>(scope->getShort(kTzKey, 0));
  if (snapshot.epoch <= 0) return std::nullopt;

  return snapshot;
}

void NvsClockRepository::save(const ClockSnapshot& snapshot) {
  if (snapshot.epoch <= 0) return;

  nvs::Scope scope(nvs::kClocks, nvs::Access::Write);
  if (!scope) return;

  scope->putLong64(kEpochKey, snapshot.epoch);
  scope->putShort(kTzKey, snapshot.tz_offset_min);
}

}  // namespace tama

#endif  // ARDUINO
