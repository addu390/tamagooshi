#include "device.h"

#include <algorithm>

namespace tama {

namespace {
void floatStarsFirst(std::vector<Metric>& metrics) {
  std::stable_sort(metrics.begin(), metrics.end(), [](const Metric& a, const Metric& b) {
    return (a.kind == MetricKind::Star) && (b.kind != MetricKind::Star);
  });
}

int severityRank(Severity s) {
  switch (s) {
    case Severity::Critical: return 3;
    case Severity::Warning: return 2;
    default: return 1;
  }
}
}  // namespace

void DeviceState::upsertMetric(const Metric& m) {
  for (auto& existing : metrics) {
    if (existing.key == m.key) {
      existing = m;
      floatStarsFirst(metrics);
      return;
    }
  }
  metrics.push_back(m);
  floatStarsFirst(metrics);
}

const Metric* DeviceState::starMetric() const {
  for (const auto& m : metrics) {
    if (m.kind == MetricKind::Star) return &m;
  }
  return metrics.empty() ? nullptr : &metrics.front();
}

const Metric* DeviceState::metricByKey(const std::string& key) const {
  for (const auto& m : metrics) {
    if (m.key == key) return &m;
  }
  return nullptr;
}

void DeviceState::raisePrompt(const Page& page) {
  for (auto& p : pending_) {
    if (p.source == page.source) {
      p = page;
      selectPrompt();
      return;
    }
  }
  pending_.push_back(page);
  selectPrompt();
}

void DeviceState::clearPrompt(const std::string& id) {
  for (auto it = pending_.begin(); it != pending_.end(); ++it) {
    if (it->id == id) {
      pending_.erase(it);
      break;
    }
  }
  selectPrompt();
}

void DeviceState::clearPromptSource(const std::string& source) {
  for (auto it = pending_.begin(); it != pending_.end(); ++it) {
    if (it->source == source) {
      pending_.erase(it);
      break;
    }
  }
  selectPrompt();
}

void DeviceState::selectPrompt() {
  const Page* best = nullptr;
  for (const auto& p : pending_) {
    if (!best) {
      best = &p;
      continue;
    }
    const int rank = severityRank(p.severity);
    const int bestRank = severityRank(best->severity);
    if (rank > bestRank) {
      best = &p;
    } else if (rank == bestRank && prompt && prompt->id == p.id) {
      best = &p;
    }
  }
  if (best) {
    prompt = *best;
  } else {
    prompt.reset();
  }
}

}  // namespace tama
