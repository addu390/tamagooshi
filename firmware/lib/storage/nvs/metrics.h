#pragma once

#include "metric.h"

namespace tama {

class NvsMetricRepository : public IMetricRepository {
 public:
  std::vector<Metric> load() override;
  void save(const std::vector<Metric>& metrics) override;
};

}  // namespace tama
