#include <string>

#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

constexpr uint8_t kBrightLevels[] = {60, 120, 180, 255};
constexpr int kBrightCount = sizeof(kBrightLevels) / sizeof(kBrightLevels[0]);

void cycleBrightness(ShellContext& ctx) {
  int i = 0;
  for (int k = 0; k < kBrightCount; ++k) {
    if (kBrightLevels[k] == ctx.state.brightness) i = k;
  }
  ctx.state.brightness = kBrightLevels[(i + 1) % kBrightCount];
}

using ValueFn = std::string (*)(ShellContext&);
using ActFn = Transition (*)(ShellContext&);

struct SettingItem {
  const char* label;
  ValueFn value;
  ActFn activate;
};

struct SettingGroup {
  const char* label;
  const char* id;
  const SettingItem* items;
  int count;
};

std::string valMascot(ShellContext& c) { return widgets::upper(c.character ? c.character->name() : "-"); }
std::string valTheme(ShellContext&) { return widgets::upper(theme::name(theme::current())); }
std::string valFont(ShellContext&) { return widgets::upper(typeface::name(typeface::current())); }
std::string valOrient(ShellContext& c) {
  return c.state.orientation == Orientation::Landscape ? "HORIZ" : "VERT";
}
std::string valBright(ShellContext& c) {
  return std::to_string((c.state.brightness * 100) / 255) + "%";
}
std::string valMute(ShellContext& c) { return c.state.muted ? "ON" : "OFF"; }

std::string valBt(ShellContext& c) {
  if (!c.link.available()) return "-";
  if (!c.link.enabled()) return "OFF";
  return c.link.connected() ? "LINKED" : "ON";
}

std::string valWifi(ShellContext& c) {
  if (!c.wifi || !c.wifi->available()) return "-";
  if (!c.wifi->enabled()) return "OFF";
  const std::string p = c.wifi->peer();
  return p.empty() ? "ON" : widgets::upper(p.c_str());
}

Transition actMascot(ShellContext&) { return Transition::push("mascots"); }
Transition actTheme(ShellContext& c) {
  theme::setTheme(c.state.enabled.nextTheme(theme::count(), theme::current(), theme::name));
  return Transition::redraw();
}
Transition actFont(ShellContext& c) {
  typeface::setTypeface(
      c.state.enabled.nextTypeface(typeface::count(), typeface::current(), typeface::name));
  return Transition::redraw();
}
Transition actOrient(ShellContext& c) {
  c.state.orientation = c.state.orientation == Orientation::Portrait ? Orientation::Landscape
                                                                     : Orientation::Portrait;
  return Transition::redraw();
}
Transition actBright(ShellContext& c) {
  cycleBrightness(c);
  return Transition::redraw();
}
Transition actMute(ShellContext& c) {
  c.state.muted = !c.state.muted;
  return Transition::redraw();
}
Transition actBt(ShellContext&) { return Transition::push("bluetooth"); }
Transition actWifi(ShellContext&) { return Transition::push("wifi"); }

constexpr SettingItem kDisplay[] = {
    {"ORIENT", valOrient, actOrient},
    {"BRIGHT", valBright, actBright},
};
constexpr SettingItem kAppearance[] = {
    {"MASCOT", valMascot, actMascot},
    {"THEME", valTheme, actTheme},
    {"FONT", valFont, actFont},
};
constexpr SettingItem kConnect[] = {
    {"BLUETOOTH", valBt, actBt},
#if defined(TAMA_ENABLE_WIFI)
    {"WIFI", valWifi, actWifi},
#endif
};
constexpr int kConnectCount = sizeof(kConnect) / sizeof(kConnect[0]);
constexpr SettingItem kSound[] = {
    {"MUTE", valMute, actMute},
};

constexpr SettingGroup kGroups[] = {
    {"DISPLAY", "set.display", kDisplay, 2},
    {"APPEARANCE", "set.appearance", kAppearance, 3},
    {"CONNECT", "set.connect", kConnect, kConnectCount},
    {"SOUND", "set.sound", kSound, 1},
};
constexpr int kGroupCount = sizeof(kGroups) / sizeof(kGroups[0]);

class SettingsGroupScreen : public AppScreen {
 public:
  void bind(const SettingGroup* group) { group_ = group; }
  const char* id() const override { return group_->id; }
  void onEnter(ShellContext&) override { row_ = 0; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, group_->label);

    std::string vals[kMaxItems];
    widgets::ListItem rows[kMaxItems];
    for (int i = 0; i < group_->count; ++i) {
      vals[i] = group_->items[i].value(ctx);
      rows[i] = {group_->items[i].label, vals[i].c_str(), true};
    }
    widgets::listView(g, L, rows, group_->count, row_);
    widgets::hints(g, "CHANGE", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      row_ = cycleIndex(intent, row_, group_->count);
      return Transition::redraw();
    }
    if (intent == Intent::Select) return group_->items[row_].activate(ctx);
    return Transition::none();
  }

 private:
  static constexpr int kMaxItems = 6;
  const SettingGroup* group_ = nullptr;
  int row_ = 0;
};

class SettingsScreen : public AppScreen {
 public:
  const char* id() const override { return "settings"; }
  void onEnter(ShellContext&) override { row_ = 0; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "SETTINGS");

    widgets::ListItem rows[kGroupCount];
    for (int i = 0; i < kGroupCount; ++i) rows[i] = {kGroups[i].label, ">", true};
    widgets::listView(g, L, rows, kGroupCount, row_);
    widgets::hints(g, "OPEN", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next || intent == Intent::Prev) {
      row_ = cycleIndex(intent, row_, kGroupCount);
      return Transition::redraw();
    }
    if (intent == Intent::Select) return Transition::push(kGroups[row_].id);
    return Transition::none();
  }

 private:
  int row_ = 0;
};

}  // namespace

AppScreen& settings() {
  static SettingsScreen instance;
  return instance;
}

void addSettingsScreens(Navigator& nav) {
  nav.add(settings());
  static SettingsGroupScreen groups[kGroupCount];
  for (int i = 0; i < kGroupCount; ++i) {
    groups[i].bind(&kGroups[i]);
    nav.add(groups[i]);
  }
}

}  // namespace tama::screens
