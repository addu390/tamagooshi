#include "nvs/ircodes.h"

#ifdef ARDUINO

#include <algorithm>
#include <string>

#include "nvs/scope.h"

namespace tama {

namespace {
std::string labelKey(int i) { return "label" + std::to_string(i); }
std::string frameKey(int i) { return "frame" + std::to_string(i); }
}  // namespace

int NvsIrCodeRepository::load(IrButton* out, int max) {
  nvs::Scope scope(nvs::kIrCodes, nvs::Access::Read);
  if (!scope) return 0;
  const int count = std::min<int>(scope->getUChar("count", 0), max);
  int n = 0;
  for (int i = 0; i < count; ++i) {
    IrButton& b = out[n];
    b.label = scope->getString(labelKey(i).c_str(), "").c_str();
    const size_t bytes =
        scope->getBytes(frameKey(i).c_str(), b.frame.pulses, sizeof(b.frame.pulses));
    b.frame.count = static_cast<uint8_t>(bytes / sizeof(b.frame.pulses[0]));
    if (!b.label.empty() && !b.frame.empty()) ++n;
  }
  return n;
}

void NvsIrCodeRepository::save(const IrButton* buttons, int count) {
  nvs::Scope scope(nvs::kIrCodes, nvs::Access::Write);
  if (!scope) return;
  scope->clear();
  scope->putUChar("count", static_cast<uint8_t>(count));
  for (int i = 0; i < count; ++i) {
    scope->putString(labelKey(i).c_str(), buttons[i].label.c_str());
    scope->putBytes(frameKey(i).c_str(), buttons[i].frame.pulses,
                    buttons[i].frame.count * sizeof(buttons[i].frame.pulses[0]));
  }
}

}  // namespace tama

#endif  // ARDUINO
