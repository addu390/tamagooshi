#pragma once

#include <cstdint>
#include <string>
#include <vector>

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

class IMetricRepository {
 public:
  virtual ~IMetricRepository() = default;
  virtual std::vector<Metric> load() = 0;
  virtual void save(const std::vector<Metric>& metrics) = 0;
};

class NullMetricRepository : public IMetricRepository {
 public:
  std::vector<Metric> load() override { return {}; }
  void save(const std::vector<Metric>&) override {}
};

}  // namespace tama
