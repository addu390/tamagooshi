#include "catalog.h"

namespace tama::screens {

int CatalogScreen::rows(ShellContext& ctx, widgets::ListItem* out, int max) {
  CatalogEntry items[kMaxRows];
  const int n = entries(ctx, items, max < kMaxRows ? max : kMaxRows);
  for (int i = 0; i < n; ++i) {
    const bool lk = items[i].locked;
    out[i] = {items[i].label, lk ? (items[i].note ? items[i].note : "LOCKED") : "", !lk};
  }
  return n;
}

Transition CatalogScreen::activate(int row, ShellContext& ctx) {
  CatalogEntry items[kMaxRows];
  const int n = entries(ctx, items, kMaxRows);
  if (row < n && !items[row].locked && items[row].screen) {
    return Transition::push(items[row].screen);
  }
  return Transition::none();
}

}  // namespace tama::screens
