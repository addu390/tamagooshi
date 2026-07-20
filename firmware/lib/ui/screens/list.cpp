#include "list.h"

#include "theme.h"

namespace tama::screens {

void ListScreen::render(Gfx& g, ShellContext& ctx) {
  const auto L = widgets::frame(g, ctx.state, section());

  if (!available(ctx)) {
    g.str("unavailable", L.cx, L.cy, theme::kDim, typeface::body(), textdatum_t::middle_center);
    widgets::hints(g, "", "BACK");
    return;
  }

  if (confirm_.active()) {
    confirm_.render(g, L);
    return;
  }

  widgets::ListItem items[kMaxRows];
  const int n = rows(ctx, items, kMaxRows);
  if (sel_ >= n) sel_ = n > 0 ? n - 1 : 0;
  widgets::listView(g, L, items, n, sel_);
  renderBelow(g, L, ctx, n);
  widgets::hints(g, actionHint(), "NEXT");
}

Transition ListScreen::handleInput(Intent intent, ShellContext& ctx) {
  if (!available(ctx)) return Transition::none();

  if (confirm_.active()) {
    if (intent == Intent::Select) {
      confirm_.cancel();
      return onConfirm(ctx);
    }
    if (intent == Intent::Next || intent == Intent::Prev) {
      confirm_.cancel();
      return Transition::redraw();
    }
    return Transition::none();
  }

  widgets::ListItem items[kMaxRows];
  const int n = rows(ctx, items, kMaxRows);
  if (intent == Intent::Next || intent == Intent::Prev) {
    sel_ = cycleIndex(intent, sel_, n);
    return Transition::redraw();
  }
  if (intent == Intent::Select && sel_ < n) return activate(sel_, ctx);
  return Transition::none();
}

}  // namespace tama::screens
