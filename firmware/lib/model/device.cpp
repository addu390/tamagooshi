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

void DeviceState::publishMetric(const Metric& m) {
  metrics_dirty = true;
  dirty = true;
  bool replaced = false;
  for (auto& existing : metrics) {
    if (existing.key == m.key) {
      existing = m;
      replaced = true;
      break;
    }
  }
  if (!replaced) metrics.push_back(m);
  floatStarsFirst(metrics);
  react();
}

void DeviceState::removeMetric(const std::string& key) {
  for (auto it = metrics.begin(); it != metrics.end(); ++it) {
    if (it->key == key) {
      metrics.erase(it);
      metrics_dirty = true;
      react();
      return;
    }
  }
}

void DeviceState::clearMetrics() {
  if (metrics.empty()) return;
  metrics.clear();
  metrics_dirty = true;
  dirty = true;
  react();
}

void DeviceState::react() {
  const MoodRule* best = nullptr;
  for (const auto& rule : reactions.moods) {
    if ((!best || rule.priority > best->priority) && rule.when.matches(metrics)) best = &rule;
  }
  setMood(best ? best->mood : reactions.fallback);

  for (const auto& rule : reactions.alerts) {
    const auto it = std::find(raised_.begin(), raised_.end(), rule.id);
    const bool was = it != raised_.end();
    const bool now = rule.when.matches(metrics);
    if (now && !was) {
      raised_.push_back(rule.id);
      raisePrompt(rule.page());
    } else if (!now && was) {
      raised_.erase(it);
      clearPrompt(rule.id);
    }
  }
}

void DeviceState::setMood(Mood m) {
  if (m == mood) return;
  mood = m;
  dirty = true;
}

const Metric* DeviceState::starMetric() const {
  for (const auto& m : metrics) {
    if (m.kind == MetricKind::Star) return &m;
  }
  return metrics.empty() ? nullptr : &metrics.front();
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
