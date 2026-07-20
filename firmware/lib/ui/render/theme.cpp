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
#define TAMA_ROLE(role, global) global = t.role;
  TAMA_THEME_ROLES(TAMA_ROLE)
#undef TAMA_ROLE
}

}  // namespace

#define TAMA_ROLE(role, global) uint16_t global = kThemes[0].role;
TAMA_THEME_ROLES(TAMA_ROLE)
#undef TAMA_ROLE

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

bool addRuntime(const char* name, const uint16_t colors[TAMA_THEME_ROLE_COUNT]) {
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
  slot->theme.name = slot->name;
  int i = 0;
#define TAMA_ROLE(role, global) slot->theme.role = colors[i++];
  TAMA_THEME_ROLES(TAMA_ROLE)
#undef TAMA_ROLE
  return true;
}

}  // namespace tama::theme
