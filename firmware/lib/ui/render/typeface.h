#pragma once

#include <M5Unified.h>

namespace tama::typeface {

struct Typeface {
  const char* name;
  const lgfx::IFont* micro;
  const lgfx::IFont* body;
  const lgfx::IFont* title;
  const lgfx::IFont* display;
};

int count();
int current();
const Typeface& at(int i);
const char* name(int i);
void setTypeface(int i);
bool setTypefaceByName(const char* name);

const lgfx::IFont* micro();
const lgfx::IFont* body();
const lgfx::IFont* title();
const lgfx::IFont* display();

}  // namespace tama::typeface
