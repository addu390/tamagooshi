#pragma once

#include <cstdint>

#include "screen.h"
#include "model.h"
#include "widgets.h"

namespace tama {

class PromptOverlay : public AppScreen {
 public:
  const char* id() const override { return "prompt"; }

  void set(const Page& page);
  bool buzzes() const;

  void render(Gfx& g, ShellContext& ctx) override;
  Transition handleInput(Intent intent, ShellContext& ctx) override;
  Transition tick(ShellContext& ctx, uint32_t nowMs) override;

 private:
  int drawHeader(Gfx& g, uint16_t col, bool landscape);
  void drawContent(Gfx& g, ShellContext& ctx, int hdrH, bool landscape);
  void drawActions(Gfx& g, bool landscape);

  Page page_;
  int action_ = 0;
  uint32_t now_ = 0;
  AnimClock anim_;
};

}  // namespace tama
