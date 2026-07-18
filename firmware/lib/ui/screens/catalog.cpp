#include "catalog.h"

#include "widgets.h"

namespace tama::screens {

void CatalogScreen::render(Gfx& g, ShellContext& ctx) {
  const auto L = widgets::frame(g, ctx.state, section());

  CatalogEntry items[kMaxItems];
  const int n = entries(ctx, items, kMaxItems);
  if (sel_ >= n) sel_ = n > 0 ? n - 1 : 0;

  widgets::ListItem rows[kMaxItems];
  for (int i = 0; i < n; ++i) {
    const bool lk = items[i].locked;
    rows[i] = {items[i].label, lk ? (items[i].note ? items[i].note : "LOCKED") : "", !lk};
  }
  widgets::listView(g, L, rows, n, sel_);
  widgets::hints(g, action(), "NEXT");
}

Transition CatalogScreen::handleInput(Intent intent, ShellContext& ctx) {
  CatalogEntry items[kMaxItems];
  const int n = entries(ctx, items, kMaxItems);
  if (intent == Intent::Next || intent == Intent::Prev) {
    sel_ = cycleIndex(intent, sel_, n);
    return Transition::redraw();
  }
  if (intent == Intent::Select && sel_ < n) {
    const CatalogEntry& e = items[sel_];
    if (!e.locked && e.screen) return Transition::push(e.screen);
  }
  return Transition::none();
}

}  // namespace tama::screens
