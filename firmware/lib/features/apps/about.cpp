#include "brand.gen.h"
#if TAMA_APP_ABOUT

#include <cstdio>
#include <string>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

std::string uptime(uint32_t secs) {
  char buf[16];
  if (secs >= 3600) {
    std::snprintf(buf, sizeof(buf), "%uh %um", secs / 3600, (secs % 3600) / 60);
  } else {
    std::snprintf(buf, sizeof(buf), "%um %02us", secs / 60, secs % 60);
  }
  return buf;
}

std::string flash(uint32_t kb) {
  char buf[16];
  if (kb == 0) {
    return "-";
  }
  if (kb >= 1024) {
    std::snprintf(buf, sizeof(buf), "%u MB", kb / 1024);
  } else {
    std::snprintf(buf, sizeof(buf), "%u KB", kb);
  }
  return buf;
}

std::string ram(uint32_t freeBytes, uint32_t totalKb) {
  char buf[24];
  const uint32_t freeKb = freeBytes / 1024;
  if (totalKb == 0) {
    if (freeKb == 0) return "-";
    std::snprintf(buf, sizeof(buf), "%u KB", freeKb);
  } else {
    std::snprintf(buf, sizeof(buf), "%u/%u KB", freeKb, totalKb);
  }
  return buf;
}

class AboutScreen : public AppScreen {
 public:
  const char* id() const override { return "app.about"; }
  OrientationPref orientation() const override { return OrientationPref::Portrait; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "ABOUT");
    const auto& sys = ctx.state.sysinfo;

    const std::string fw = sys.fw.empty() ? "-" : ("v" + sys.fw);
    widgets::heroValue(g, L.cx, L.top + 44, nullptr, fw.c_str(),
                       widgets::fitFont(g, fw.c_str(), L.w - 16, typeface::title(),
                                        typeface::body()));

    const std::string model = ctx.caps.model.empty() ? "-" : widgets::upper(ctx.caps.model.c_str());
    widgets::pill(g, L.cx, L.top + 66, model.c_str(), typeface::micro(), theme::kHi);

    const std::string flashStr = flash(sys.flash_kb);
    const std::string ramStr = ram(ctx.state.free_heap, sys.heap_total_kb);
    const std::string upStr = uptime(ctx.state.up_secs);
    const widgets::ListItem rows[] = {
        {"ID", sys.id.empty() ? "-" : sys.id.c_str()},
        {"FLASH", flashStr.c_str()},
        {"RAM", ramStr.c_str()},
        {"UPTIME", upStr.c_str()},
    };
    widgets::infoList(g, L, L.top + 92, rows, 4);
    widgets::hints(g, "", "BACK");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    return Transition::none();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    if (!anim_.due(nowMs, 1000)) return Transition::none();
    return Transition::redraw();
  }

 private:
  AnimClock anim_;
};

}  // namespace

AppScreen& about() {
  static AboutScreen instance;
  return instance;
}

}  // namespace tama::apps

#endif  // TAMA_APP_ABOUT
