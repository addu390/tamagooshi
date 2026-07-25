#pragma once

#include "screen.h"
#include "widgets.h"

namespace tama::screens {

// Standard framed list screen: section title, selectable rows cycled with B,
// activated with A, plus optional availability gate and confirm prompt.
class ListScreen : public AppScreen {
 public:
  static constexpr int kMaxRows = 24;

  void onEnter(ShellContext&) override {
    sel_ = 0;
    confirm_.cancel();
  }
  void render(Gfx& g, ShellContext& ctx) override;
  Transition handleInput(Intent intent, ShellContext& ctx) override;

 protected:
  virtual const char* section() const = 0;
  virtual const char* actionHint() const = 0;
  virtual int rows(ShellContext& ctx, widgets::ListItem* out, int max) = 0;
  virtual Transition activate(int row, ShellContext& ctx) = 0;
  virtual bool available(ShellContext&) const { return true; }
  virtual void renderBelow(Gfx&, const widgets::Layout&, ShellContext&, int rowCount) {}
  virtual Transition onConfirm(ShellContext&) { return Transition::redraw(); }

  widgets::ConfirmFlow confirm_;
  int sel_ = 0;
};

}  // namespace tama::screens
