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

const lgfx::IFont* micro() { return at(idx).micro; }
const lgfx::IFont* body() { return at(idx).body; }
const lgfx::IFont* title() { return at(idx).title; }
const lgfx::IFont* display() { return at(idx).display; }

}  // namespace tama::typeface
