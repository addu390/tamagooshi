#pragma once

#include <cstdint>

namespace tama::theme {

struct Theme {
  const char* name;
  uint16_t bg;
  uint16_t fg;
  uint16_t hi;
  uint16_t dim;
  uint16_t dimmer;
  uint16_t warn;
  uint16_t crit;
  uint16_t ink;
  uint16_t blush;
};

extern uint16_t kBg;
extern uint16_t kFg;
extern uint16_t kHi;
extern uint16_t kDim;
extern uint16_t kDimmer;
extern uint16_t kWarn;
extern uint16_t kCrit;
extern uint16_t kInk;
extern uint16_t kBlush;

int count();
int current();
const Theme& at(int i);
const char* name(int i);
void setTheme(int i);
bool setThemeByName(const char* name);

// Registers a palette from the runtime config blob. Role order matches
// tools/gen/ui/themes.py: bg, fg, hi, dim, dimmer, warn, crit, ink, blush.
bool addRuntime(const char* name, const uint16_t colors[9]);

}  // namespace tama::theme
