#pragma once

#include <string>

#include "screen.h"
#include "widgets.h"

namespace tama::screens {

// Pending yes/no prompt state rendered over a list screen.
class ConfirmFlow {
 public:
  bool active() const { return active_; }

  void arm(const char* title, std::string body) {
    active_ = true;
    title_ = title;
    body_ = std::move(body);
  }

  void cancel() { active_ = false; }

  void render(Gfx& g, const widgets::Layout& L) const {
    widgets::confirmPrompt(g, L, title_, body_.c_str());
    widgets::hints(g, "CONFIRM", "CANCEL");
  }

 private:
  bool active_ = false;
  const char* title_ = "";
  std::string body_;
};

// Standard framed list screen: section title, selectable rows cycled with B,
// activated with A, plus optional availability gate and confirm prompt.
class ListScreen : public AppScreen {
 public:
  static constexpr int kMaxRows = 8;

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

  ConfirmFlow confirm_;
  int sel_ = 0;
};

}  // namespace tama::screens
