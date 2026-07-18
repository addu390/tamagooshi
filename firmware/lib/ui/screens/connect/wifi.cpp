#include <string>

#include "screens.h"
#include "theme.h"
#include "wifi/credentials.h"
#include "widgets.h"

namespace tama::screens {

namespace {

enum class WifiAct { Toggle, Select, Forget, Provision };

class WifiScreen : public AppScreen {
 public:
  const char* id() const override { return "wifi"; }
  void onEnter(ShellContext&) override {
    row_ = 0;
    confirm_ = false;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "WIFI");

    if (!ctx.wifi || !ctx.wifi->available()) {
      g.str("unavailable", L.cx, L.cy, theme::kDim, typeface::body(), textdatum_t::middle_center);
      widgets::hints(g, "", "BACK");
      return;
    }
    IWifiControl& wifi = *ctx.wifi;

    if (confirm_) {
      widgets::confirmPrompt(g, L, "FORGET NETWORK?", forget_.c_str());
      widgets::hints(g, "CONFIRM", "CANCEL");
      return;
    }

    const int count = build(wifi);
    widgets::ListItem rows[kMax];
    for (int i = 0; i < count; ++i) rows[i] = {labels_[i].c_str(), values_[i].c_str(), true};
    if (row_ >= count) row_ = count - 1;
    widgets::listView(g, L, rows, count, row_);

    const int rowH = L.landscape ? 18 : 22;
    const int heroTop = L.top + 26 + count * rowH + 12;
    const int heroBot = L.bottom - 16;
    const int iconH = 18;
    const int iconX = L.cx - iconH / 2;

    if (!wifi.enabled()) {
      int y = (heroTop + heroBot - (iconH + widgets::kGapIcon + widgets::kLineH)) / 2;
      widgets::wifiIcon(g, iconX, y, iconH, theme::kDim);
      y += iconH + widgets::kGapIcon;
      g.str("turn on to connect", L.cx, y, theme::kDim, typeface::micro(),
            textdatum_t::top_center);
    } else if (wifi.provisioning()) {
      const std::string ap = wifi.portal();
      const lgfx::IFont* apFont =
          widgets::fitFont(g, ap.c_str(), L.w - 16, typeface::body(), typeface::micro());
      const int blockH =
          iconH + widgets::kGapIcon + widgets::kLineH + widgets::kGapLabel + widgets::kBodyH;
      int y = (heroTop + heroBot - blockH) / 2;
      widgets::wifiIcon(g, iconX, y, iconH, theme::kHi);
      y += iconH + widgets::kGapIcon;
      g.str("SETUP HOTSPOT", L.cx, y, theme::kHi, typeface::micro(), textdatum_t::top_center);
      y += widgets::kLineH + widgets::kGapLabel;
      g.str(ap.c_str(), L.cx, y, theme::kFg, apFont, textdatum_t::top_center);
      g.str("join from your phone", L.cx, L.bottom - 8, theme::kDim, typeface::micro(),
            textdatum_t::bottom_center);
    }
    widgets::hints(g, "SELECT", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (!ctx.wifi) return Transition::none();
    IWifiControl& wifi = *ctx.wifi;

    if (confirm_) {
      if (intent == Intent::Select) {
        wifi.forget(forget_);
        confirm_ = false;
        return Transition::redraw();
      }
      if (intent == Intent::Next || intent == Intent::Prev) {
        confirm_ = false;
        return Transition::redraw();
      }
      return Transition::none();
    }

    const int count = build(wifi);
    if (intent == Intent::Next || intent == Intent::Prev) {
      row_ = cycleIndex(intent, row_, count);
      return Transition::redraw();
    }
    if (intent != Intent::Select) return Transition::none();

    switch (acts_[row_]) {
      case WifiAct::Toggle:
        wifi.setEnabled(!wifi.enabled());
        break;
      case WifiAct::Select:
        wifi.select(ssids_[row_]);
        break;
      case WifiAct::Forget:
        forget_ = ssids_[row_];
        confirm_ = true;
        break;
      case WifiAct::Provision:
        wifi.provision();
        break;
    }
    return Transition::redraw();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    return anim_.due(nowMs, 500) ? Transition::redraw() : Transition::none();
  }

 private:
  static constexpr int kMax = 2 + kMaxKnownNetworks + 1;

  int build(IWifiControl& wifi) {
    int n = 0;
    labels_[n] = "POWER";
    values_[n] = wifi.enabled() ? "ON" : "OFF";
    acts_[n] = WifiAct::Toggle;
    ssids_[n].clear();
    ++n;

    if (!wifi.enabled()) return n;

    std::string active;
    for (const KnownNetwork& net : wifi.known()) {
      labels_[n] = net.ssid;
      values_[n] = net.active ? (wifi.connected() ? "LINKED" : "ACTIVE") : "";
      acts_[n] = WifiAct::Select;
      ssids_[n] = net.ssid;
      if (net.active) active = net.ssid;
      ++n;
    }

    if (!active.empty()) {
      labels_[n] = "FORGET";
      values_[n] = "";
      acts_[n] = WifiAct::Forget;
      ssids_[n] = active;
      ++n;
    }

    labels_[n] = "ADD NETWORK";
    values_[n] = "";
    acts_[n] = WifiAct::Provision;
    ssids_[n].clear();
    ++n;
    return n;
  }

  std::string labels_[kMax];
  std::string values_[kMax];
  std::string ssids_[kMax];
  WifiAct acts_[kMax] = {};
  int row_ = 0;
  bool confirm_ = false;
  std::string forget_;
  AnimClock anim_;
};

}  // namespace

AppScreen& wifi() {
  static WifiScreen instance;
  return instance;
}

}  // namespace tama::screens
