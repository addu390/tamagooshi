#include "metric.h"

namespace tama {

MetricKind metricKindFromString(const std::string& s) {
  return s == "star" ? MetricKind::Star : MetricKind::Normal;
}

}  // namespace tama
