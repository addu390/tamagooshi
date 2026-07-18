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

}  // namespace tama::theme
