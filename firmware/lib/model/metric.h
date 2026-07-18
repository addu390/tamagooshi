#pragma once

#include <cstdint>
#include <string>

namespace tama {

enum class MetricKind { Normal, Star };

MetricKind metricKindFromString(const std::string& s);

struct Metric {
  std::string key;
  std::string label;
  std::string value;
  std::string trend;
  MetricKind kind = MetricKind::Normal;
  uint32_t ts = 0;
};

}  // namespace tama
