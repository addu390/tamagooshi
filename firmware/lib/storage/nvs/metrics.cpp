#include "nvs/metrics.h"

#ifdef ARDUINO

#include <algorithm>

#include "nvs/scope.h"

namespace tama {

namespace {
constexpr size_t kMaxMetrics = 24;

std::string field(char tag, size_t i) { return std::string(1, tag) + std::to_string(i); }
}  // namespace

std::vector<Metric> NvsMetricRepository::load() {
  std::vector<Metric> list;
  nvs::Scope scope(nvs::kMetrics, nvs::Access::Read);
  if (!scope) return list;
  const size_t count = std::min<size_t>(scope->getUChar("count", 0), kMaxMetrics);
  for (size_t i = 0; i < count; ++i) {
    Metric m;
    m.key = scope->getString(field('k', i).c_str(), "").c_str();
    m.label = scope->getString(field('l', i).c_str(), "").c_str();
    m.value = scope->getString(field('v', i).c_str(), "").c_str();
    m.trend = scope->getString(field('r', i).c_str(), "").c_str();
    m.kind = scope->getUChar(field('s', i).c_str(), 0) ? MetricKind::Star : MetricKind::Normal;
    m.ts = scope->getUInt(field('t', i).c_str(), 0);
    if (!m.key.empty()) list.push_back(std::move(m));
  }
  return list;
}

void NvsMetricRepository::save(const std::vector<Metric>& metrics) {
  nvs::Scope scope(nvs::kMetrics, nvs::Access::Write);
  if (!scope) return;
  scope->clear();
  const size_t count = std::min<size_t>(metrics.size(), kMaxMetrics);
  scope->putUChar("count", static_cast<uint8_t>(count));
  for (size_t i = 0; i < count; ++i) {
    const Metric& m = metrics[i];
    scope->putString(field('k', i).c_str(), m.key.c_str());
    scope->putString(field('l', i).c_str(), m.label.c_str());
    scope->putString(field('v', i).c_str(), m.value.c_str());
    scope->putString(field('r', i).c_str(), m.trend.c_str());
    scope->putUChar(field('s', i).c_str(), m.kind == MetricKind::Star ? 1 : 0);
    scope->putUInt(field('t', i).c_str(), m.ts);
  }
}

}  // namespace tama

#endif  // ARDUINO
