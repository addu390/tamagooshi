#include "metric.h"

#include <cstdlib>

namespace tama {

MetricKind metricKindFromString(const std::string& s) {
  return s == "star" ? MetricKind::Star : MetricKind::Normal;
}

bool Metric::number(double& out) const {
  if (raw) {
    out = *raw;
    return true;
  }

  std::string digits;
  size_t i = 0;
  if (i < value.size() && (value[i] == '-' || value[i] == '+')) digits += value[i++];
  if (i < value.size() && value[i] == '$') ++i;
  for (; i < value.size(); ++i) {
    if (value[i] != ',') digits += value[i];
  }
  if (!digits.empty() && digits.back() == '%') digits.pop_back();
  if (digits.empty()) return false;

  char* end = nullptr;
  const double v = std::strtod(digits.c_str(), &end);
  if (end == digits.c_str() || *end != '\0') return false;
  out = v;
  return true;
}

}  // namespace tama
