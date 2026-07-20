#include "typeface.h"

#include <cstring>

namespace tama::typeface {

namespace {

#include "typefaces.gen.h"

constexpr int kCount = sizeof(kTypefaces) / sizeof(kTypefaces[0]);

int idx = 0;

}  // namespace

int count() { return kCount; }
int current() { return idx; }
const Typeface& at(int i) { return kTypefaces[((i % kCount) + kCount) % kCount]; }
const char* name(int i) { return at(i).name; }

void setTypeface(int i) { idx = ((i % kCount) + kCount) % kCount; }

bool setTypefaceByName(const char* name) {
  if (!name) return false;
  for (int i = 0; i < kCount; ++i) {
    if (std::strcmp(kTypefaces[i].name, name) == 0) {
      setTypeface(i);
      return true;
    }
  }
  return false;
}

#define TAMA_ROLE(role) \
  const lgfx::IFont* role() { return at(idx).role; }
TAMA_TYPEFACE_ROLES(TAMA_ROLE)
#undef TAMA_ROLE

}  // namespace tama::typeface
