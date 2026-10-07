#include "reactions.h"

namespace tama {

namespace {

const Metric* find(const std::vector<Metric>& metrics, const std::string& key) {
  for (const auto& m : metrics) {
    if (m.key == key) return &m;
  }
  return nullptr;
}

}  // namespace

bool ruleOpFromString(const std::string& s, RuleOp& out) {
  if (s == "lt") out = RuleOp::Lt;
  else if (s == "lte") out = RuleOp::Lte;
  else if (s == "gt") out = RuleOp::Gt;
  else if (s == "gte") out = RuleOp::Gte;
  else if (s == "eq") out = RuleOp::Eq;
  else if (s == "ne") out = RuleOp::Ne;
  else return false;
  return true;
}

bool Condition::matches(const std::vector<Metric>& metrics) const {
  const Metric* m = find(metrics, metric);
  double v = 0;
  if (!m || !m->number(v)) return false;
  switch (op) {
    case RuleOp::Lt: return v < value;
    case RuleOp::Lte: return v <= value;
    case RuleOp::Gt: return v > value;
    case RuleOp::Gte: return v >= value;
    case RuleOp::Eq: return v == value;
    case RuleOp::Ne: return v != value;
  }
  return false;
}

Page AlertRule::page() const {
  Page p;
  p.id = id;
  p.severity = severity;
  p.title = title;
  p.body = body;
  p.source = source;
  p.requires_ack = requires_ack;
  p.actions[0] = {"ACK", PromptOutcome::Ack};
  p.actions[1] = {"SNOOZE", PromptOutcome::Snooze};
  p.actionCount = 2;
  return p;
}

}  // namespace tama
