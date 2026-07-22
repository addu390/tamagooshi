#pragma once

#include <cstdint>
#include <string>

#include "anim.h"
#include "gfx.h"
#include "model.h"

namespace tama {

namespace widgets {

constexpr int kGapIcon = 16;
constexpr int kGapLine = 6;
constexpr int kGapLabel = 4;
constexpr int kLineH = 12;
constexpr int kBodyH = 16;

struct Layout {
  int w;
  int h;
  int top;
  int bottom;
  int cx;
  int cy;
  bool landscape;
  int contentH() const { return bottom - top; }
};

Layout layout(Gfx& g, bool withStatusbar = true, bool withHints = true);

inline const lgfx::IFont* uiFont(bool small) {
  return small ? typeface::micro() : typeface::body();
}

template <typename... Fonts>
const lgfx::IFont* fitFont(Gfx& g, const char* s, int maxWidth, const Fonts*... ladder) {
  const lgfx::IFont* fonts[] = {static_cast<const lgfx::IFont*>(ladder)...};
  for (const lgfx::IFont* font : fonts) {
    if (g.textWidth(s, font) <= maxWidth) return font;
  }
  return fonts[sizeof...(ladder) - 1];
}

enum class Side { Left, Center, Right };

int anchor(const Layout& L, Side side);

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

struct SelectStyle {
  int radius = 7;
  uint16_t fill = theme::kFg;
  uint16_t outline = theme::kDim;
  uint16_t selectedOutline = 0;
  uint16_t content = theme::kFg;
  uint16_t selectedContent = theme::kBg;
};

uint16_t selectionBox(Gfx& g, const Rect& r, bool selected, const SelectStyle& style = {});

struct Grid {
  int cols;
  int rows;
  int tileW;
  int tileH;
  int gap;
  int top;
  int fullW;

  Rect cell(int index, int count) const;
};

Grid grid(const Layout& L, int count, int colsPortrait, int colsLandscape, int top, int gap,
          int pad, int maxTileH);

void statusbar(Gfx& g, const DeviceState& state, bool showBrand = true);
Layout frame(Gfx& g, const DeviceState& state, const char* section = nullptr, bool showBrand = true);
void bluetoothIcon(Gfx& g, int x, int y, int h, uint16_t col);
void wifiIcon(Gfx& g, int x, int y, int h, uint16_t col);
void brandLockup(Gfx& g, const Branding& brand, int cx, int cy, int maxWidth, int maxHeight = 40);
int mascotNameY(int mascotCy, int size);
void mascotLabel(Gfx& g, const char* name, int cx, int mascotCy, int size);
int wrapText(Gfx& g, const char* text, int cx, int y, int maxWidth, const lgfx::IFont* font,
             uint16_t color, int lineH, int maxY = 0);
void hints(Gfx& g, const char* a, const char* b);
void sectionLabel(Gfx& g, const char* text);
std::string upper(const char* s);
void title(Gfx& g, const char* s, int x, int y, int maxWidth, uint16_t color, textdatum_t datum);
void confirmPrompt(Gfx& g, const Layout& L, const char* title, const char* sub = nullptr);
void trend(Gfx& g, int cx, int cy, char dir);
void pill(Gfx& g, int cx, int y, const char* text, const lgfx::IFont* font, uint16_t color = theme::kFg);
void heroValue(Gfx& g, int cx, int cy, const char* label, const char* value,
               const lgfx::IFont* valueFont, int labelGap = 6);
const lgfx::IFont* heroFont(Gfx& g, const char* value, int maxWidth);
void toggle(Gfx& g, int rightX, int cy, bool on, uint16_t accent);
void gauge(Gfx& g, int rightX, int cy, int width, int pct, uint16_t accent);

struct StatStyle {
  const lgfx::IFont* font = nullptr;
  bool rounded = false;
  int critThreshold = 0;
  int labelW = 0;
  int barH = 7;
};

enum class RowVisual : uint8_t { Text, Toggle, Gauge };

struct ListItem {
  const char* label;
  const char* value = "";
  bool enabled = true;
  RowVisual visual = RowVisual::Text;
  int level = 0;
};

void listRow(Gfx& g, const Rect& r, const ListItem& item, bool selected, const lgfx::IFont* font);

void listView(Gfx& g, const Layout& L, const ListItem* items, int count, int sel);
void infoList(Gfx& g, const Layout& L, int top, const ListItem* items, int count);

void statBar(Gfx& g, int y, const char* label, int pct);
void statBarAt(Gfx& g, int x, int y, int width, const char* label, int pct,
               const StatStyle& style = {});
void dots(Gfx& g, int cx, int y, int count, int active);

}  // namespace widgets
}  // namespace tama
