#include "pager.h"

namespace tama {

Severity severityFromString(const std::string& s) {
  if (s == "critical") return Severity::Critical;
  if (s == "warning") return Severity::Warning;
  return Severity::Info;
}

const char* severityToString(Severity s) {
  switch (s) {
    case Severity::Critical: return "critical";
    case Severity::Warning: return "warning";
    default: return "info";
  }
}

}  // namespace tama
