#include "apps/apps.h"
#include "catalog.h"
#include "screens.h"

namespace tama::screens {

namespace {

class AppsScreen : public CatalogScreen {
 public:
  const char* id() const override { return "apps"; }

 protected:
  const char* section() const override { return "APPS"; }
  const char* action() const override { return "OPEN"; }

  int entries(ShellContext& ctx, CatalogEntry* out, int max) const override {
    const int total = apps::count();
    const FeatureInfo* items = apps::list();
    int n = 0;
    for (int i = 0; i < total && n < max; ++i) {
      if (!ctx.state.enabled.app(items[i].id)) continue;
      out[n++] = {items[i].label, items[i].screen, items[i].note, locked(items[i], ctx.caps)};
    }
    return n;
  }
};

}  // namespace

AppScreen& apps() {
  static AppsScreen instance;
  return instance;
}

}  // namespace tama::screens
