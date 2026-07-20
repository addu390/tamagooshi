#pragma once

#include "list.h"

namespace tama::screens {

struct CatalogEntry {
  const char* label;
  const char* screen;
  const char* note;
  bool locked;
};

// List of navigable features; locked entries show their note and stay put.
class CatalogScreen : public ListScreen {
 protected:
  virtual const char* action() const = 0;
  virtual int entries(ShellContext& ctx, CatalogEntry* out, int max) const = 0;

  const char* actionHint() const override { return action(); }
  int rows(ShellContext& ctx, widgets::ListItem* out, int max) override;
  Transition activate(int row, ShellContext& ctx) override;
};

}  // namespace tama::screens
