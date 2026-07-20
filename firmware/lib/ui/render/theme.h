#pragma once

#include <cstdint>

#include "theme_roles.gen.h"

namespace tama::theme {

struct Theme {
  const char* name;
#define TAMA_ROLE(role, global) uint16_t role;
  TAMA_THEME_ROLES(TAMA_ROLE)
#undef TAMA_ROLE
};

#define TAMA_ROLE(role, global) extern uint16_t global;
TAMA_THEME_ROLES(TAMA_ROLE)
#undef TAMA_ROLE

int count();
int current();
const Theme& at(int i);
const char* name(int i);
void setTheme(int i);
bool setThemeByName(const char* name);

// Registers a palette from the runtime config blob. Role order matches
// tools/gen/ui/themes.py ROLES.
bool addRuntime(const char* name, const uint16_t colors[TAMA_THEME_ROLE_COUNT]);

}  // namespace tama::theme
