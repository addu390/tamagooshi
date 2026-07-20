#include <string>

#include "list.h"
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
using LevelFn = int (*)(ShellContext&);

struct SettingItem {
  const char* label;
  ValueFn value;
  ActFn activate;
  widgets::RowVisual visual = widgets::RowVisual::Text;
  LevelFn level = nullptr;
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

int lvlBright(ShellContext& c) { return (c.state.brightness * 100) / 255; }
int lvlMute(ShellContext& c) { return c.state.muted ? 100 : 0; }

constexpr SettingItem kDisplay[] = {
    {"ORIENT", valOrient, actOrient},
    {"BRIGHT", valBright, actBright, widgets::RowVisual::Gauge, lvlBright},
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
    {"MUTE", valMute, actMute, widgets::RowVisual::Toggle, lvlMute},
};

constexpr SettingGroup kGroups[] = {
    {"DISPLAY", "set.display", kDisplay, 2},
    {"APPEARANCE", "set.appearance", kAppearance, 3},
    {"CONNECT", "set.connect", kConnect, kConnectCount},
    {"SOUND", "set.sound", kSound, 1},
};
constexpr int kGroupCount = sizeof(kGroups) / sizeof(kGroups[0]);

class SettingsGroupScreen : public ListScreen {
 public:
  void bind(const SettingGroup* group) { group_ = group; }
  const char* id() const override { return group_->id; }

 protected:
  const char* section() const override { return group_->label; }
  const char* actionHint() const override { return "CHANGE"; }

  int rows(ShellContext& ctx, widgets::ListItem* out, int) override {
    for (int i = 0; i < group_->count; ++i) {
      const SettingItem& item = group_->items[i];
      vals_[i] = item.value(ctx);
      out[i] = {item.label, vals_[i].c_str(), true, item.visual, item.level ? item.level(ctx) : 0};
    }
    return group_->count;
  }

  Transition activate(int row, ShellContext& ctx) override {
    return group_->items[row].activate(ctx);
  }

 private:
  const SettingGroup* group_ = nullptr;
  std::string vals_[kMaxRows];
};

class SettingsScreen : public ListScreen {
 public:
  const char* id() const override { return "settings"; }

 protected:
  const char* section() const override { return "SETTINGS"; }
  const char* actionHint() const override { return "OPEN"; }

  int rows(ShellContext&, widgets::ListItem* out, int) override {
    for (int i = 0; i < kGroupCount; ++i) out[i] = {kGroups[i].label, ">", true};
    return kGroupCount;
  }

  Transition activate(int row, ShellContext&) override { return Transition::push(kGroups[row].id); }
};

}  // namespace

TAMA_SCREEN_FACTORY(settings, SettingsScreen)

void addSettingsScreens(Navigator& nav) {
  nav.add(settings());
  static SettingsGroupScreen groups[kGroupCount];
  for (int i = 0; i < kGroupCount; ++i) {
    groups[i].bind(&kGroups[i]);
    nav.add(groups[i]);
  }
}

}  // namespace tama::screens
