#pragma once

#include "navigator.h"
#include "screen.h"

namespace tama {

struct GameInfo {
  const char* id;
  const char* label;
  const char* screen;
  bool needsJoystick;
  bool needsImu;
  bool needsMic;
  const char* note;
};

}  // namespace tama

namespace tama::games {

AppScreen& runner();
AppScreen& flappy();
AppScreen& delivery();
AppScreen& scream();
AppScreen& galaxy();

const GameInfo* list();
int count();
void registerAll(Navigator& nav);

}  // namespace tama::games
