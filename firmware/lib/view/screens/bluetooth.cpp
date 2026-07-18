#include <cstdio>
#include <string>

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

class BluetoothScreen : public AppScreen {
 public:
  const char* id() const override { return "bluetooth"; }
  void onEnter(ShellContext&) override {
    row_ = 0;
    confirm_ = false;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    ILink& link = ctx.link;
    const auto L = widgets::frame(g, ctx.state, "BLUETOOTH");

    if (!link.available()) {
      g.str("unavailable", L.cx, L.cy, theme::kDim, typeface::body(), textdatum_t::middle_center);
      widgets::hints(g, "", "BACK");
      return;
    }

    if (confirm_) {
      widgets::confirmPrompt(g, L, "FORGET DEVICE?", "erases pairing");
      widgets::hints(g, "CONFIRM", "CANCEL");
      return;
    }

    const int count = build(ctx);
    widgets::ListItem rows[kMax];
    for (int i = 0; i < count; ++i) rows[i] = {labels_[i], values_[i].c_str(), true};
    if (row_ >= count) row_ = count - 1;
    widgets::listView(g, L, rows, count, row_);

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
      const int blockH =
          iconH + widgets::kGapIcon + widgets::kBodyH + (p.empty() ? 0 : widgets::kGapLine + widgets::kLineH);
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
    widgets::hints(g, "SELECT", "NEXT");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (confirm_) {
      if (intent == Intent::Select) {
        ctx.link.unpair();
        confirm_ = false;
        return Transition::redraw();
      }
      if (intent == Intent::Next || intent == Intent::Prev) {
        confirm_ = false;
        return Transition::redraw();
      }
      return Transition::none();
    }

    const int count = build(ctx);
    if (intent == Intent::Next || intent == Intent::Prev) {
      row_ = cycleIndex(intent, row_, count);
      return Transition::redraw();
    }
    if (intent == Intent::Select) {
      switch (acts_[row_]) {
        case BtAction::Toggle:
          ctx.link.setEnabled(!ctx.link.enabled());
          return Transition::redraw();
        case BtAction::Forget:
          confirm_ = true;
          return Transition::redraw();
      }
    }
    return Transition::none();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    return anim_.due(nowMs, 500) ? Transition::redraw() : Transition::none();
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
  int row_ = 0;
  bool confirm_ = false;
  AnimClock anim_;
};

}  // namespace

AppScreen& bluetooth() {
  static BluetoothScreen instance;
  return instance;
}

}  // namespace tama::screens
