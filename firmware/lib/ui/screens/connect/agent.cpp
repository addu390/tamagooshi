#include <string>

#include "list.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

const char* agentStatus(AgentState state) {
  switch (state) {
    case AgentState::Unpaired: return "UNPAIRED";
    case AgentState::Pairing: return "PAIRING";
    case AgentState::Confirming: return "CONFIRM";
    case AgentState::Joining: return "JOINING";
    case AgentState::Connecting: return "CONNECTING";
    case AgentState::Online: return "ONLINE";
  }
  return "-";
}

namespace {

bool pairing(AgentState s) {
  return s == AgentState::Pairing || s == AgentState::Confirming || s == AgentState::Joining;
}

class AgentScreen : public ListScreen {
 public:
  const char* id() const override { return "agent"; }
  uint32_t redrawPeriodMs() const override { return 500; }

 protected:
  const char* section() const override { return "AGENT"; }
  const char* actionHint() const override { return "SELECT"; }
  bool available(ShellContext& ctx) const override { return ctx.assistant != nullptr; }

  int rows(ShellContext& ctx, widgets::ListItem* out, int) override {
    const AgentState s = ctx.assistant->state();
    name_ = widgets::upper(ctx.assistant->name().c_str());
    if (s == AgentState::Unpaired) {
      out[0] = {"PAIR", ""};
    } else if (pairing(s)) {
      out[0] = {"CANCEL", ""};
    } else {
      out[0] = {"UNPAIR", name_.c_str()};
    }
    out[1] = {"STATUS", agentStatus(s), false};
    return 2;
  }

  void renderBelow(Gfx& g, const widgets::Layout& L, ShellContext& ctx, int count) override {
    const AgentState s = ctx.assistant->state();
    if (!pairing(s)) return;
    const int rowH = L.landscape ? 18 : 22;
    const int top = L.top + 26 + count * rowH + 12;
    const int blockH = widgets::kLineH + widgets::kGapLabel + widgets::kBodyH;
    const int y = (top + L.bottom - 16 - blockH) / 2;
    const std::string beacon = ctx.assistant->beacon();
    const lgfx::IFont* font =
        widgets::fitFont(g, beacon.c_str(), L.w - 16, typeface::body(), typeface::micro());
    g.str(s == AgentState::Confirming ? "CONFIRM ON DEVICE" : "OPEN THE APP", L.cx, y, theme::kHi,
          typeface::micro(), textdatum_t::top_center);
    g.str(beacon.c_str(), L.cx, y + widgets::kLineH + widgets::kGapLabel, theme::kFg, font,
          textdatum_t::top_center);
  }

  Transition activate(int row, ShellContext& ctx) override {
    if (row != 0) return Transition::none();
    IAgent& agent = *ctx.assistant;
    const AgentState s = agent.state();
    if (s == AgentState::Unpaired) {
      agent.pair();
    } else if (pairing(s)) {
      agent.cancel();
    } else {
      confirm_.arm("UNPAIR AGENT?", name_);
    }
    return Transition::redraw();
  }

  Transition onConfirm(ShellContext& ctx) override {
    ctx.assistant->unpair();
    return Transition::redraw();
  }

 private:
  std::string name_;
};

}  // namespace

TAMA_SCREEN_FACTORY(agent, AgentScreen)

}  // namespace tama::screens
