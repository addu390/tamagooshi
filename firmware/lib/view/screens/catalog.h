#pragma once

#include "screen.h"

namespace tama::screens {

struct CatalogEntry {
  const char* label;
  const char* screen;
  const char* note;
  bool locked;
};

class CatalogScreen : public AppScreen {
 public:
  static constexpr int kMaxItems = 8;

  void onEnter(ShellContext&) override { sel_ = 0; }
  void render(Gfx& g, ShellContext& ctx) override;
  Transition handleInput(Intent intent, ShellContext& ctx) override;

 protected:
  virtual const char* section() const = 0;
  virtual const char* action() const = 0;
  virtual int entries(ShellContext& ctx, CatalogEntry* out, int max) const = 0;

 private:
  int sel_ = 0;
};

}  // namespace tama::screens
