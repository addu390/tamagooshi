#include "catalog.h"
#include "games/games.h"
#include "screens.h"

namespace tama::screens {

namespace {

bool locked(const GameInfo& gi, const DeviceCapabilities& caps) {
  if (!gi.screen) return true;
  if (gi.needsJoystick && !caps.joystick) return true;
  if (gi.needsImu && !caps.imu) return true;
  if (gi.needsMic && !caps.mic) return true;
  return false;
}

class PlayScreen : public CatalogScreen {
 public:
  const char* id() const override { return "play"; }

 protected:
  const char* section() const override { return "PLAY"; }
  const char* action() const override { return "GO"; }

  int entries(ShellContext& ctx, CatalogEntry* out, int max) const override {
    const int total = games::count();
    const GameInfo* items = games::list();
    int n = 0;
    for (int i = 0; i < total && n < max; ++i) {
      if (!ctx.state.enabled.game(items[i].id)) continue;
      out[n++] = {items[i].label, items[i].screen, items[i].note, locked(items[i], ctx.caps)};
    }
    return n;
  }
};

}  // namespace

AppScreen& play() {
  static PlayScreen instance;
  return instance;
}

}  // namespace tama::screens
