#include "theme.h"

#include <cstring>

namespace tama::theme {

namespace {

#include "themes.gen.h"

constexpr int kBuiltinCount = sizeof(kThemes) / sizeof(kThemes[0]);
constexpr int kRuntimeCap = 8;

struct RuntimeSlot {
  char name[24];
  Theme theme;
};

RuntimeSlot runtimeSlots[kRuntimeCap];
int runtimeCount = 0;

int idx = 0;

int total() { return kBuiltinCount + runtimeCount; }

const Theme& themeAt(int i) {
  const int n = total();
  i = ((i % n) + n) % n;
  return i < kBuiltinCount ? kThemes[i] : runtimeSlots[i - kBuiltinCount].theme;
}

void apply(const Theme& t) {
  kBg = t.bg;
  kFg = t.fg;
  kHi = t.hi;
  kDim = t.dim;
  kDimmer = t.dimmer;
  kWarn = t.warn;
  kCrit = t.crit;
  kInk = t.ink;
  kBlush = t.blush;
}

}  // namespace

uint16_t kBg = 0xE75C;
uint16_t kFg = 0x2146;
uint16_t kHi = 0x2146;
uint16_t kDim = 0x7BF1;
uint16_t kDimmer = 0xBE18;
uint16_t kWarn = 0xD445;
uint16_t kCrit = 0xC1C5;
uint16_t kInk = 0x2146;
uint16_t kBlush = 0xFCB0;

int count() { return total(); }
int current() { return idx; }
const Theme& at(int i) { return themeAt(i); }
const char* name(int i) { return themeAt(i).name; }

void setTheme(int i) {
  const int n = total();
  idx = ((i % n) + n) % n;
  apply(themeAt(idx));
}

bool setThemeByName(const char* name) {
  if (!name) return false;
  for (int i = 0; i < total(); ++i) {
    if (std::strcmp(themeAt(i).name, name) == 0) {
      setTheme(i);
      return true;
    }
  }
  return false;
}

bool addRuntime(const char* name, const uint16_t colors[9]) {
  if (!name || !*name || !colors) return false;
  for (int i = 0; i < kBuiltinCount; ++i) {
    if (std::strcmp(kThemes[i].name, name) == 0) return false;
  }

  RuntimeSlot* slot = nullptr;
  for (int i = 0; i < runtimeCount; ++i) {
    if (std::strcmp(runtimeSlots[i].name, name) == 0) {
      slot = &runtimeSlots[i];
      break;
    }
  }
  if (slot == nullptr) {
    if (runtimeCount >= kRuntimeCap) return false;
    slot = &runtimeSlots[runtimeCount++];
  }

  std::strncpy(slot->name, name, sizeof(slot->name) - 1);
  slot->name[sizeof(slot->name) - 1] = '\0';
  slot->theme = {slot->name, colors[0], colors[1], colors[2], colors[3], colors[4],
                 colors[5], colors[6], colors[7], colors[8]};
  return true;
}

}  // namespace tama::theme
