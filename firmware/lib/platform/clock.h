#pragma once

#include <cstdint>
#include <optional>

namespace tama {

struct ClockSnapshot {
  int64_t epoch = 0;
  int16_t tz_offset_min = 0;
};

class IClockRepository {
 public:
  virtual ~IClockRepository() = default;
  virtual std::optional<ClockSnapshot> load() = 0;
  virtual void save(const ClockSnapshot& clock) = 0;
};

class NullClockRepository : public IClockRepository {
 public:
  std::optional<ClockSnapshot> load() override { return std::nullopt; }
  void save(const ClockSnapshot&) override {}
};

}  // namespace tama
