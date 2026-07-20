#pragma once

#include "navigator.h"
#include "screen.h"

namespace tama {

struct AppInfo {
  const char* id;
  const char* label;
  const char* screen;
  bool needsImu;
  const char* note;
};

}  // namespace tama

namespace tama::apps {

AppScreen& clock();
AppScreen& stopwatch();
AppScreen& flashlight();
AppScreen& level();
AppScreen& pomodoro();
AppScreen& about();

const AppInfo* list();
int count();
void registerAll(Navigator& nav);

}  // namespace tama::apps
