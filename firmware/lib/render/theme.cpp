#include "theme.h"

#include <cstring>

namespace tama::theme {

namespace {

#include "themes.gen.h"

constexpr int kCount = sizeof(kThemes) / sizeof(kThemes[0]);

int idx = 0;

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

int count() { return kCount; }
int current() { return idx; }
const Theme& at(int i) { return kThemes[((i % kCount) + kCount) % kCount]; }
const char* name(int i) { return at(i).name; }

void setTheme(int i) {
  idx = ((i % kCount) + kCount) % kCount;
  apply(kThemes[idx]);
}

bool setThemeByName(const char* name) {
  if (!name) return false;
  for (int i = 0; i < kCount; ++i) {
    if (std::strcmp(kThemes[i].name, name) == 0) {
      setTheme(i);
      return true;
    }
  }
  return false;
}

}  // namespace tama::theme
