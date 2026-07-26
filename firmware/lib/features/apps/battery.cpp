#include "brand.gen.h"
#if TAMA_APP_BATTERY

#include <algorithm>
#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

class BatteryScreen : public AppScreen {
 public:
  const char* id() const override { return "app.battery"; }
  OrientationPref orientation() const override { return OrientationPref::Portrait; }

  void onEnter(ShellContext& ctx) override { sample(ctx); }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "BATTERY");
    const int pct = std::max(0, std::min(100, sample_.batt_pct));

    char pctStr[8];
    std::snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
    widgets::heroValue(g, L.cx, L.top + 44, nullptr, pctStr,
                       widgets::heroFont(g, pctStr, L.w - 16));

    widgets::pill(g, L.cx, L.top + 70, sample_.usb ? "CHARGING" : "BATTERY", typeface::micro(),
                  sample_.usb ? theme::kHi : theme::kDim);

    const int barY = L.top + 92;
    widgets::StatStyle bar;
    bar.font = typeface::micro();
    bar.rounded = true;
    bar.critThreshold = 15;
    bar.barH = 9;
    widgets::statBarAt(g, 16, barY, L.w - 32, "LEVEL", pct, bar);

    char volt[16];
    if (sample_.mV > 0) {
      std::snprintf(volt, sizeof(volt), "%d.%02d V", sample_.mV / 1000, (sample_.mV % 1000) / 10);
    } else {
      std::snprintf(volt, sizeof(volt), "-");
    }

    char curr[16];
    if (sample_.mA != 0) {
      std::snprintf(curr, sizeof(curr), "%+d mA", sample_.mA);
    } else if (sample_.mV > 0) {
      std::snprintf(curr, sizeof(curr), "0 mA");
    } else {
      std::snprintf(curr, sizeof(curr), "-");
    }

    const widgets::ListItem rows[] = {
        {"VOLTAGE", volt},
        {"CURRENT", curr},
        {"USB", sample_.usb ? "YES" : "NO"},
    };
    widgets::infoList(g, L, barY + 28, rows, 3);
    widgets::hints(g, "", "BACK");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (!anim_.due(nowMs, 1000)) return Transition::none();
    sample(ctx);
    return Transition::redraw();
  }

 private:
  void sample(ShellContext& ctx) { sample_ = ctx.telemetry.read(); }

  AnimClock anim_;
  Telemetry sample_;
};

}  // namespace

TAMA_SCREEN_FACTORY(battery, BatteryScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_BATTERY
