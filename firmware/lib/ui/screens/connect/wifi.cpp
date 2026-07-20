#include <string>

#include "list.h"
#include "screens.h"
#include "theme.h"
#include "wifi/credentials.h"
#include "widgets.h"

namespace tama::screens {

namespace {

enum class WifiAct { Toggle, Select, Forget, Provision };

class WifiScreen : public ListScreen {
 public:
  const char* id() const override { return "wifi"; }
  uint32_t redrawPeriodMs() const override { return 500; }

 protected:
  const char* section() const override { return "WIFI"; }
  const char* actionHint() const override { return "SELECT"; }
  bool available(ShellContext& ctx) const override { return ctx.wifi && ctx.wifi->available(); }

  int rows(ShellContext& ctx, widgets::ListItem* out, int) override {
    const int n = build(*ctx.wifi);
    for (int i = 0; i < n; ++i) out[i] = {labels_[i].c_str(), values_[i].c_str(), true};
    return n;
  }

  void renderBelow(Gfx& g, const widgets::Layout& L, ShellContext& ctx, int count) override {
    IWifiControl& wifi = *ctx.wifi;
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
  }

  Transition activate(int row, ShellContext& ctx) override {
    IWifiControl& wifi = *ctx.wifi;
    switch (acts_[row]) {
      case WifiAct::Toggle:
        wifi.setEnabled(!wifi.enabled());
        break;
      case WifiAct::Select:
        wifi.select(ssids_[row]);
        break;
      case WifiAct::Forget:
        forget_ = ssids_[row];
        confirm_.arm("FORGET NETWORK?", forget_);
        break;
      case WifiAct::Provision:
        wifi.provision();
        break;
    }
    return Transition::redraw();
  }

  Transition onConfirm(ShellContext& ctx) override {
    ctx.wifi->forget(forget_);
    return Transition::redraw();
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
  std::string forget_;
};

}  // namespace

TAMA_SCREEN_FACTORY(wifi, WifiScreen)

}  // namespace tama::screens
