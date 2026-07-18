#include "prompt_overlay.h"

#include <string>

#include "mascot.h"

namespace tama {

namespace {

uint16_t severityColor(Severity s) {
  switch (s) {
    case Severity::Critical: return theme::kCrit;
    case Severity::Warning: return theme::kWarn;
    default: return theme::kFg;
  }
}

bool isApproval(const Page& page) {
  for (int i = 0; i < page.actionCount; ++i) {
    if (page.actions[i].outcome == PromptOutcome::Allow) return true;
  }
  return false;
}

}  // namespace

void PromptOverlay::set(const Page& page) {
  page_ = page;
  action_ = 0;
}

bool PromptOverlay::buzzes() const {
  return page_.requires_ack && page_.severity != Severity::Info;
}

void PromptOverlay::render(Gfx& g, ShellContext& ctx) {
  const uint16_t col = severityColor(page_.severity);
  const bool landscape = g.w() > g.h();

  const int hdrH = drawHeader(g, col, landscape);
  drawContent(g, ctx, hdrH, landscape);
  drawActions(g, landscape);

  widgets::hints(g, "SELECT", "NEXT");
}

int PromptOverlay::drawHeader(Gfx& g, uint16_t col, bool landscape) {
  const int hdrH = landscape ? 20 : 24;
  g.c().fillRect(0, 0, g.w(), hdrH, col);
  const char* header = isApproval(page_) ? "NEEDS YOU" : "! PAGE !";
  g.str(header, g.w() / 2, hdrH / 2, theme::kInk, typeface::body(), textdatum_t::middle_center);

  g.str(severityToString(page_.severity), 6, hdrH + 6, col, typeface::body(),
        textdatum_t::top_left);
  if (!page_.source.empty()) {
    g.str(page_.source.c_str(), g.w() - 6, hdrH + 6, theme::kDim, typeface::body(),
          textdatum_t::top_right);
  }
  return hdrH;
}

void PromptOverlay::drawContent(Gfx& g, ShellContext& ctx, int hdrH, bool landscape) {
  const int titleY = hdrH + (landscape ? 26 : 28);
  const int bodyY = hdrH + (landscape ? 44 : 48);
  const int cx = g.w() / 2;
  widgets::title(g, page_.title.c_str(), cx, titleY, g.w() - 12, theme::kHi,
                 textdatum_t::top_center);
  if (!page_.body.empty()) {
    const int wrapCx = landscape ? (g.w() - 60) / 2 : cx;
    const int wrapW = landscape ? g.w() - 72 : g.w() - 12;
    const int maxY = landscape ? g.h() - 42 : g.h() / 2 - 18;
    widgets::wrapText(g, page_.body.c_str(), wrapCx, bodyY, wrapW, typeface::micro(),
                      theme::kDim, 11, maxY);
  }

  if (!ctx.character) return;
  if (landscape) {
    ctx.character->draw(g, g.w() - 34, hdrH + 42, 38, MascotState{Expr::Alert}, now_);
  } else {
    ctx.character->draw(g, g.w() / 2, g.h() / 2 + 16, 44, MascotState{Expr::Alert}, now_);
  }
}

void PromptOverlay::drawActions(Gfx& g, bool landscape) {
  const int chipW = 58;
  const int gap = 6;
  const int totalW = page_.actionCount * chipW + (page_.actionCount - 1) * gap;
  int x = g.w() / 2 - totalW / 2;
  const int y = g.h() - (landscape ? 38 : 46);

  widgets::SelectStyle chip;
  chip.radius = 4;
  chip.fill = theme::kDimmer;
  chip.outline = theme::kDim;
  chip.selectedOutline = theme::kFg;
  chip.content = theme::kDim;
  chip.selectedContent = theme::kHi;
  for (int i = 0; i < page_.actionCount; ++i) {
    const uint16_t fg = widgets::selectionBox(g, {x, y, chipW, 18}, i == action_, chip);
    g.str(page_.actions[i].label, x + chipW / 2, y + 9, fg, typeface::body(),
          textdatum_t::middle_center);
    x += chipW + gap;
  }
}

Transition PromptOverlay::handleInput(Intent intent, ShellContext& ctx) {
  if (page_.actionCount == 0) return Transition::none();
  if (intent == Intent::Next || intent == Intent::Prev) {
    action_ = cycleIndex(intent, action_, page_.actionCount);
    return Transition::redraw();
  }
  if (intent == Intent::Select) {
    if (ctx.resolvePrompt) ctx.resolvePrompt(page_, page_.actions[action_].outcome);
    return Transition::back();
  }
  return Transition::none();
}

Transition PromptOverlay::tick(ShellContext&, uint32_t nowMs) {
  now_ = nowMs;
  return anim_.due(nowMs, 55) ? Transition::redraw() : Transition::none();
}

}  // namespace tama
