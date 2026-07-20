#pragma once

#include <M5Unified.h>

#include "typeface_roles.gen.h"

namespace tama::typeface {

struct Typeface {
  const char* name;
#define TAMA_ROLE(role) const lgfx::IFont* role;
  TAMA_TYPEFACE_ROLES(TAMA_ROLE)
#undef TAMA_ROLE
};

int count();
int current();
const Typeface& at(int i);
const char* name(int i);
void setTypeface(int i);
bool setTypefaceByName(const char* name);

#define TAMA_ROLE(role) const lgfx::IFont* role();
TAMA_TYPEFACE_ROLES(TAMA_ROLE)
#undef TAMA_ROLE

}  // namespace tama::typeface
