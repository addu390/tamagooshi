#pragma once

#include <cstdint>
#include <string>

namespace tama {

enum class Severity { Info, Warning, Critical };

Severity severityFromString(const std::string& s);
const char* severityToString(Severity s);

enum class PromptOutcome { Ack, Snooze, Allow, Deny };

struct PromptAction {
  const char* label = nullptr;
  PromptOutcome outcome = PromptOutcome::Ack;
};

struct Page {
  std::string id;
  Severity severity = Severity::Info;
  std::string title;
  std::string body;
  std::string source;
  uint32_t ts = 0;
  bool requires_ack = true;
  PromptAction actions[2];
  int actionCount = 0;
};

}  // namespace tama
