#include "apps.h"

#include "brand.gen.h"

namespace tama::apps {

namespace {

constexpr AppInfo kApps[] = {
#if TAMA_APP_CLOCK
    {"clock", "CLOCK", "app.clock", false, nullptr},
#endif
#if TAMA_APP_STOPWATCH
    {"stopwatch", "STOPWATCH", "app.stopwatch", false, nullptr},
#endif
#if TAMA_APP_FLASHLIGHT
    {"flashlight", "TORCH", "app.flashlight", false, nullptr},
#endif
#if TAMA_APP_LEVEL
    {"level", "LEVEL", "app.level", true, nullptr},
#endif
#if TAMA_APP_POMODORO
    {"pomodoro", "POMODORO", "app.pomodoro", false, nullptr},
#endif
#if TAMA_APP_ABOUT
    {"about", "ABOUT", "app.about", false, nullptr},
#endif
};

}  // namespace

const AppInfo* list() { return kApps; }

int count() { return sizeof(kApps) / sizeof(kApps[0]); }

void registerAll(Navigator& nav) {
#if TAMA_APP_CLOCK
  nav.add(clock());
#endif
#if TAMA_APP_STOPWATCH
  nav.add(stopwatch());
#endif
#if TAMA_APP_FLASHLIGHT
  nav.add(flashlight());
#endif
#if TAMA_APP_LEVEL
  nav.add(level());
#endif
#if TAMA_APP_POMODORO
  nav.add(pomodoro());
#endif
#if TAMA_APP_ABOUT
  nav.add(about());
#endif
}

}  // namespace tama::apps
