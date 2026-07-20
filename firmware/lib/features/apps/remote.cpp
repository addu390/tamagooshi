#include "brand.gen.h"
#if TAMA_APP_REMOTE

#include <string>

#include "apps.h"
#include "list.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kMaxButtons = screens::ListScreen::kMaxRows - 2;
constexpr uint32_t kSentFlashMs = 900;

class RemoteScreen : public screens::ListScreen {
 public:
  const char* id() const override { return "app.remote"; }
  uint32_t redrawPeriodMs() const override { return 250; }

  void onEnter(ShellContext& ctx) override {
    ListScreen::onEnter(ctx);
    learning_ = false;
    sentRow_ = -1;
    count_ = ctx.irStore ? ctx.irStore->load(buttons_, kMaxButtons) : 0;
  }

  void onExit() override {
    if (learning_ && ir_) ir_->stopLearn();
    learning_ = false;
    ir_ = nullptr;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    if (!learning_) {
      ListScreen::render(g, ctx);
      return;
    }
    const auto L = widgets::frame(g, ctx.state, section());
    const int cy = L.top + L.contentH() / 2;
    widgets::pill(g, L.cx, cy - 34, "LEARNING", typeface::micro(), theme::kHi);
    g.str("point remote", L.cx, cy, theme::kFg, typeface::body(), textdatum_t::middle_center);
    g.str("press a button", L.cx, cy + 18, theme::kDim, typeface::micro(),
          textdatum_t::middle_center);
    widgets::hints(g, "", "CANCEL");
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    if (!learning_) return ListScreen::handleInput(intent, ctx);
    if (intent == Intent::Select || intent == Intent::Next) {
      stopLearning();
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    const Transition base = ListScreen::tick(ctx, nowMs);
    if (!learning_ || !ctx.ir) return base;

    IrFrame frame;
    if (!ctx.ir->fetchLearned(frame)) return base;
    stopLearning();
    buttons_[count_].label = "BTN " + std::to_string(count_ + 1);
    buttons_[count_].frame = frame;
    ++count_;
    if (ctx.irStore) ctx.irStore->save(buttons_, count_);
    return Transition::redraw();
  }

 protected:
  const char* section() const override { return "REMOTE"; }
  const char* actionHint() const override { return "SEND"; }
  bool available(ShellContext& ctx) const override { return ctx.ir && ctx.irStore; }

  int rows(ShellContext&, widgets::ListItem* out, int) override {
    int n = 0;
    const bool flash = sentRow_ >= 0 && now() < sentUntil_;
    for (; n < count_; ++n) {
      out[n] = {buttons_[n].label.c_str(), flash && n == sentRow_ ? "SENT" : "", true};
    }
    if (count_ < kMaxButtons) out[n++] = {"LEARN", "+", true};
    if (count_ > 0) out[n++] = {"CLEAR ALL", "", true};
    return n;
  }

  Transition activate(int row, ShellContext& ctx) override {
    if (row < count_) {
      ctx.ir->send(buttons_[row].frame);
      sentRow_ = row;
      sentUntil_ = now() + kSentFlashMs;
      return Transition::redraw();
    }
    if (row == count_ && count_ < kMaxButtons) {
      ir_ = ctx.ir;
      ir_->startLearn();
      learning_ = true;
      return Transition::redraw();
    }
    confirm_.arm("CLEAR BUTTONS?", "forget all learned codes");
    return Transition::redraw();
  }

  Transition onConfirm(ShellContext& ctx) override {
    count_ = 0;
    sentRow_ = -1;
    if (ctx.irStore) ctx.irStore->save(buttons_, 0);
    return Transition::redraw();
  }

 private:
  void stopLearning() {
    if (ir_) ir_->stopLearn();
    learning_ = false;
  }

  IrButton buttons_[kMaxButtons];
  int count_ = 0;
  bool learning_ = false;
  IIrTransceiver* ir_ = nullptr;
  int sentRow_ = -1;
  uint32_t sentUntil_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(remote, RemoteScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_REMOTE
