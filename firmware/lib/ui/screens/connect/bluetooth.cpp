#include <cstdio>
#include <string>

#include "list.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

std::string passkeyText(uint32_t code) {
  if (code == 0) return "------";
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%06u", code % 1000000u);
  std::string s(buf);
  return s.substr(0, 3) + " " + s.substr(3);
}

enum class BtAction { Toggle, Forget };

class BluetoothScreen : public ListScreen {
 public:
  const char* id() const override { return "bluetooth"; }
  uint32_t redrawPeriodMs() const override { return 500; }

 protected:
  const char* section() const override { return "BLUETOOTH"; }
  const char* actionHint() const override { return "SELECT"; }
  bool available(ShellContext& ctx) const override { return ctx.link.available(); }

  int rows(ShellContext& ctx, widgets::ListItem* out, int) override {
    const int n = build(ctx);
    for (int i = 0; i < n; ++i) out[i] = {labels_[i], values_[i].c_str(), true};
    return n;
  }

  void renderBelow(Gfx& g, const widgets::Layout& L, ShellContext& ctx, int count) override {
    ILink& link = ctx.link;
    const bool on = link.enabled();
    const bool paired = on && link.paired();
    const int rowH = L.landscape ? 18 : 22;
    const int heroTop = L.top + 26 + count * rowH + 12;
    const int heroBot = L.bottom - 16;
    const int iconH = 24;
    const uint16_t glyphCol = paired ? theme::kHi : (on ? theme::kFg : theme::kDim);
    const int iconX = L.cx - iconH / 4;

    if (!on) {
      int y = (heroTop + heroBot - (iconH + widgets::kGapIcon + widgets::kLineH)) / 2;
      widgets::bluetoothIcon(g, iconX, y, iconH, glyphCol);
      y += iconH + widgets::kGapIcon;
      g.str("turn on to pair", L.cx, y, theme::kDim, typeface::micro(), textdatum_t::top_center);
    } else if (paired) {
      const std::string p = link.peer();
      const int blockH = iconH + widgets::kGapIcon + widgets::kBodyH +
                         (p.empty() ? 0 : widgets::kGapLine + widgets::kLineH);
      int y = (heroTop + heroBot - blockH) / 2;
      widgets::bluetoothIcon(g, iconX, y, iconH, glyphCol);
      y += iconH + widgets::kGapIcon;
      g.str("LINKED", L.cx, y, theme::kHi, typeface::body(), textdatum_t::top_center);
      if (!p.empty()) {
        y += widgets::kBodyH + widgets::kGapLine;
        g.str(p.c_str(), L.cx, y, theme::kDim, typeface::micro(), textdatum_t::top_center);
      }
    } else {
      const std::string code = passkeyText(link.passkey());
      const lgfx::IFont* codeFont =
          widgets::fitFont(g, code.c_str(), L.w - 16, typeface::title(), typeface::body());
      g.c().setFont(codeFont);
      const int codeH = g.c().fontHeight();
      const int blockH = iconH + widgets::kGapIcon + widgets::kLineH + widgets::kGapLabel + codeH;
      int y = (heroTop + heroBot - blockH) / 2;
      widgets::bluetoothIcon(g, iconX, y, iconH, glyphCol);
      y += iconH + widgets::kGapIcon;
      g.str("PASSKEY", L.cx, y, theme::kDim, typeface::micro(), textdatum_t::top_center);
      y += widgets::kLineH + widgets::kGapLabel;
      g.str(code.c_str(), L.cx, y, theme::kFg, codeFont, textdatum_t::top_center);
    }

    const std::string name = link.deviceName();
    if (on && !paired) {
      g.str("enter on your phone", L.cx, L.bottom - 8, theme::kDim, typeface::micro(),
            textdatum_t::bottom_center);
    } else if (!name.empty()) {
      g.str(name.c_str(), L.cx, L.bottom - 8, theme::kDim, typeface::micro(),
            textdatum_t::bottom_center);
    }
  }

  Transition activate(int row, ShellContext& ctx) override {
    switch (acts_[row]) {
      case BtAction::Toggle:
        ctx.link.setEnabled(!ctx.link.enabled());
        return Transition::redraw();
      case BtAction::Forget:
        confirm_.arm("FORGET DEVICE?", "erases pairing");
        return Transition::redraw();
    }
    return Transition::none();
  }

  Transition onConfirm(ShellContext& ctx) override {
    ctx.link.unpair();
    return Transition::redraw();
  }

 private:
  static constexpr int kMax = 2;

  int build(ShellContext& ctx) {
    ILink& link = ctx.link;
    int n = 0;
    labels_[n] = "POWER";
    values_[n] = link.enabled() ? "ON" : "OFF";
    acts_[n] = BtAction::Toggle;
    ++n;

    if (link.enabled() && link.paired()) {
      labels_[n] = "FORGET";
      values_[n] = "";
      acts_[n] = BtAction::Forget;
      ++n;
    }
    return n;
  }

  const char* labels_[kMax] = {};
  std::string values_[kMax];
  BtAction acts_[kMax] = {};
};

}  // namespace

TAMA_SCREEN_FACTORY(bluetooth, BluetoothScreen)

}  // namespace tama::screens
