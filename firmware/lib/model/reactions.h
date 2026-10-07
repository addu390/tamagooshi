#pragma once

#include <string>
#include <vector>

#include "mascot.h"
#include "metric.h"
#include "pager.h"

namespace tama {

enum class RuleOp { Lt, Lte, Gt, Gte, Eq, Ne };

bool ruleOpFromString(const std::string& s, RuleOp& out);

struct Condition {
  std::string metric;
  RuleOp op = RuleOp::Gt;
  double value = 0;

  bool matches(const std::vector<Metric>& metrics) const;
};

struct MoodRule {
  Condition when;
  Mood mood = Mood::Happy;
  int priority = 0;
};

struct AlertRule {
  std::string id;
  Condition when;
  Severity severity = Severity::Warning;
  std::string title;
  std::string body;
  std::string source = "rules";
  bool requires_ack = true;

  Page page() const;
};

struct Reactions {
  std::vector<MoodRule> moods;
  std::vector<AlertRule> alerts;
  Mood fallback = Mood::Happy;
};

}  // namespace tama
