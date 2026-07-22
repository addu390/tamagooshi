#include "widgets.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "logos.h"

namespace tama::widgets {

std::string upper(const char* s) {
  std::string out(s ? s : "");
  for (char& c : out) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return out;
}

Layout layout(Gfx& g, bool withStatusbar, bool withHints) {
  Layout l;
  l.w = g.w();
  l.h = g.h();
  l.top = withStatusbar ? 24 : 4;
  l.bottom = l.h - (withHints ? 24 : 4);
  l.cx = l.w / 2;
  l.cy = (l.top + l.bottom) / 2;
  l.landscape = l.w > l.h;
  return l;
}

int anchor(const Layout& L, Side side) {
  switch (side) {
    case Side::Left: return L.w / 4;
    case Side::Right: return L.w * 3 / 4;
    default: return L.cx;
  }
}

uint16_t selectionBox(Gfx& g, const Rect& r, bool selected, const SelectStyle& s) {
  auto& c = g.c();
  if (selected) {
    c.fillRoundRect(r.x, r.y, r.w, r.h, s.radius, s.fill);
    if (s.selectedOutline) c.drawRoundRect(r.x, r.y, r.w, r.h, s.radius, s.selectedOutline);
    return s.selectedContent;
  }
  if (s.outline) c.drawRoundRect(r.x, r.y, r.w, r.h, s.radius, s.outline);
  return s.content;
}

Rect Grid::cell(int index, int count) const {
  const int col = index % cols;
  const int row = index / cols;
  const int inRow = (row == rows - 1) ? (count - row * cols) : cols;
  const int rowW = inRow * tileW + (inRow - 1) * gap;
  const int rowX = (fullW - rowW) / 2;
  return {rowX + col * (tileW + gap), top + row * (tileH + gap), tileW, tileH};
}

Grid grid(const Layout& L, int count, int colsPortrait, int colsLandscape, int top, int gap,
          int pad, int maxTileH) {
  Grid gr;
  gr.cols = L.landscape ? colsLandscape : colsPortrait;
  gr.rows = (count + gr.cols - 1) / gr.cols;
  gr.gap = gap;
  gr.top = top;
  gr.fullW = L.w;
  gr.tileW = (L.w - pad * 2 - gap * (gr.cols - 1)) / gr.cols;
  const int avail = L.bottom - top;
  int th = (avail - gap * (gr.rows - 1)) / gr.rows;
  if (maxTileH > 0 && th > maxTileH) th = maxTileH;
  gr.tileH = th;
  return gr;
}

namespace {

void drawBolt(M5Canvas& c, int cx, int y, int h, uint16_t col) {
  c.drawLine(cx + 1, y + 1, cx - 2, y + h / 2, col);
  c.drawLine(cx - 2, y + h / 2, cx + 1, y + h / 2, col);
  c.drawLine(cx + 1, y + h / 2, cx - 1, y + h - 1, col);
}

int drawBattery(Gfx& g, int rightX, const DeviceState& state) {
  auto& c = g.c();
  const int w = 22, h = 11, y = 2;
  const int x = rightX - w;
  c.drawRoundRect(x, y, w, h, 2, theme::kDim);
  c.fillRect(x + w, y + 3, 2, h - 6, theme::kDim);

  const int level = std::max(0, std::min(100, state.batt_pct));
  const int fill = (w - 4) * level / 100;
  if (fill > 0) c.fillRect(x + 2, y + 2, fill, h - 4, theme::kFg);
  if (state.charging) drawBolt(c, x + w / 2, y, h, theme::kHi);
  return x;
}

int drawLink(Gfx& g, int rightX, bool connected) {
  const int w = 9, x = rightX - w;
  const uint16_t col = connected ? theme::kFg : theme::kDim;
#if defined(TAMA_ENABLE_WIFI) && !defined(TAMA_ENABLE_BLE)
  wifiIcon(g, x, 2, 11, col);
#else
  bluetoothIcon(g, x, 2, 11, col);
#endif
  return x;
}

}  // namespace

void bluetoothIcon(Gfx& g, int x, int y, int h, uint16_t col) {
  auto& c = g.c();
  const int cx = x + h / 4, r = x + h / 2;
  const int top = y, bot = y + h, q1 = y + h / 4, q3 = y + 3 * h / 4;
  c.drawLine(cx, top, cx, bot, col);
  c.drawLine(cx, top, r, q1, col);
  c.drawLine(r, q1, x, q3, col);
  c.drawLine(cx, bot, r, q3, col);
  c.drawLine(r, q3, x, q1, col);
}

void wifiIcon(Gfx& g, int x, int y, int h, uint16_t col) {
  auto& c = g.c();
  const int cx = x + h / 2, base = y + h - 1;
  c.fillCircle(cx, base, 1, col);
  c.drawArc(cx, base, h / 3, h / 3, 210, 330, col);
  c.drawArc(cx, base, 2 * h / 3, 2 * h / 3, 210, 330, col);
}

void statusbar(Gfx& g, const DeviceState& state, bool showBrand) {
  const int battLeft = drawBattery(g, g.w() - 4, state);
  const int linkLeft = drawLink(g, battLeft - 6, state.connected);
  int rightEdge = linkLeft;
  if (state.muted) {
    g.str("M", linkLeft - 4, 2, theme::kDim, typeface::body(), textdatum_t::top_right);
    rightEdge = linkLeft - 12;
  }

  const std::string& label = showBrand ? state.branding.name : state.branding.dev_name;
  if (!label.empty()) {
    const int maxW = rightEdge - 10;
    g.str(label.c_str(), 4, 2, theme::kFg,
          fitFont(g, label.c_str(), maxW, typeface::body(), typeface::micro()),
          textdatum_t::top_left);
  }

  g.c().drawFastHLine(0, 20, g.w(), theme::kDim);
}

Layout frame(Gfx& g, const DeviceState& state, const char* section, bool showBrand) {
  statusbar(g, state, showBrand);
  if (section) sectionLabel(g, section);
  return layout(g);
}

int mascotNameY(int mascotCy, int size) {
  const int base = size / 22 < 1 ? 1 : size / 22;
  return mascotCy + 16 * base + 8;
}

void mascotLabel(Gfx& g, const char* name, int cx, int mascotCy, int size) {
  if (!name || !name[0]) return;
  g.str(name, cx, mascotNameY(mascotCy, size), theme::kDim, typeface::micro(),
        textdatum_t::top_center);
}

void brandLockup(Gfx& g, const Branding& brand, int cx, int cy, int maxWidth, int maxHeight) {
  auto& c = g.c();
  const bool hasLogo = !brand.logo_id.empty() && logos::has(brand.logo_id);
  const char* name = brand.name.c_str();

  c.setFont(typeface::title());
  c.setTextSize(1.0f);
  const int textW1 = c.textWidth(name);
  const int textH1 = c.fontHeight();
  const int logoH1 = hasLogo ? textH1 * 3 / 4 : 0;
  const int span1 = logos::width(logoH1) + (hasLogo ? logoH1 / 3 : 0) + textW1;

  float scale = span1 > 0 ? static_cast<float>(maxWidth) / static_cast<float>(span1) : 1.0f;
  if (textH1 * scale > maxHeight) scale = static_cast<float>(maxHeight) / static_cast<float>(textH1);

  c.setTextSize(scale);
  const int textW = c.textWidth(name);
  const int textH = c.fontHeight();
  const int logoH = hasLogo ? textH * 3 / 4 : 0;
  const int logoW = logos::width(logoH);
  const int gap = hasLogo ? logoH / 3 : 0;
  const int yTop = cy - textH / 2;

  int x = cx - (logoW + gap + textW) / 2;
  if (hasLogo) {
    logos::draw(g, brand.logo_id, x + logoW / 2, yTop + logoH / 2, logoH, theme::kHi);
    x += logoW + gap;
  }
  c.setTextDatum(textdatum_t::top_left);
  c.setTextColor(theme::kFg);
  c.drawString(name, x, yTop);
  c.setTextSize(1.0f);
}

int wrapText(Gfx& g, const char* text, int cx, int y, int maxWidth, const lgfx::IFont* font,
             uint16_t color, int lineH, int maxY) {
  const std::string s(text);
  std::string line;
  int cy = y;
  size_t start = 0;
  const auto lastLine = [&](int nextY) { return maxY > 0 && nextY + lineH > maxY; };
  while (start <= s.size()) {
    const size_t sp = s.find(' ', start);
    const std::string word = s.substr(start, sp == std::string::npos ? sp : sp - start);
    const std::string cand = line.empty() ? word : line + " " + word;
    if (line.empty() || g.textWidth(cand.c_str(), font) <= maxWidth) {
      line = cand;
    } else {
      if (lastLine(cy)) {
        g.str((line + "...").c_str(), cx, cy, color, font, textdatum_t::top_center);
        return cy + lineH;
      }
      g.str(line.c_str(), cx, cy, color, font, textdatum_t::top_center);
      cy += lineH;
      line = word;
    }
    if (sp == std::string::npos) break;
    start = sp + 1;
  }
  if (!line.empty()) {
    g.str(line.c_str(), cx, cy, color, font, textdatum_t::top_center);
    cy += lineH;
  }
  return cy;
}

namespace {

void hintBadge(Gfx& g, int x, int cy, char key) {
  auto& c = g.c();
  const int s = 12;
  c.fillRoundRect(x, cy - s / 2, s, s, 3, theme::kDim);
  const char label[2] = {key, '\0'};
  g.str(label, x + s / 2, cy, theme::kBg, typeface::micro(), textdatum_t::middle_center);
}

}  // namespace

void hints(Gfx& g, const char* a, const char* b) {
  const int cy = g.h() - 12;
  g.c().drawFastHLine(0, cy - 11, g.w(), theme::kDimmer);
  if (a && a[0]) {
    hintBadge(g, 4, cy, 'A');
    g.str(a, 20, cy, theme::kDim, typeface::micro(), textdatum_t::middle_left);
  }
  if (b && b[0]) {
    hintBadge(g, g.w() - 16, cy, 'B');
    g.str(b, g.w() - 20, cy, theme::kDim, typeface::micro(), textdatum_t::middle_right);
  }
}

void sectionLabel(Gfx& g, const char* text) {
  auto& c = g.c();
  c.setFont(typeface::micro());
  const int tw = c.textWidth(text);
  const int cx = g.w() / 2;
  const int cy = 30;
  g.str(text, cx, cy, theme::kHi, typeface::micro(), textdatum_t::middle_center);
  const int arm = (g.w() - tw) / 2 - 16;
  if (arm > 4) {
    c.drawFastHLine(cx - tw / 2 - 8 - arm, cy, arm, theme::kDimmer);
    c.drawFastHLine(cx + tw / 2 + 8, cy, arm, theme::kDimmer);
  }
}

void title(Gfx& g, const char* s, int x, int y, int maxWidth, uint16_t color, textdatum_t datum) {
  g.str(s, x, y, color, fitFont(g, s, maxWidth, typeface::title(), typeface::body(),
                                typeface::micro()),
        datum);
}

void toggle(Gfx& g, int rightX, int cy, bool on, uint16_t accent) {
  auto& c = g.c();
  const int w = 24, h = 12, r = h / 2;
  const int x = rightX - w;
  const int y = cy - h / 2;
  if (on) {
    c.fillRoundRect(x, y, w, h, r, accent);
    c.fillCircle(x + w - r - 1, cy, r - 3, theme::kBg);
  } else {
    c.drawRoundRect(x, y, w, h, r, theme::kDim);
    c.fillCircle(x + r + 1, cy, r - 3, theme::kDim);
  }
}

void gauge(Gfx& g, int rightX, int cy, int width, int pct, uint16_t accent) {
  auto& c = g.c();
  const int segs = 5;
  const int gap = 2;
  const int segW = (width - gap * (segs - 1)) / segs;
  const int h = 8;
  const int lit = (std::max(0, std::min(100, pct)) * segs + 99) / 100;
  int x = rightX - segW * segs - gap * (segs - 1);
  for (int i = 0; i < segs; ++i) {
    if (i < lit) {
      c.fillRoundRect(x, cy - h / 2, segW, h, 2, accent);
    } else {
      c.drawRoundRect(x, cy - h / 2, segW, h, 2, theme::kDim);
    }
    x += segW + gap;
  }
}

void listRow(Gfx& g, const Rect& r, const ListItem& item, bool selected, const lgfx::IFont* font) {
  SelectStyle style;
  style.radius = 4;
  style.outline = 0;
  if (!item.enabled) style.fill = theme::kDim;
  const uint16_t val = selectionBox(g, r, selected, style);
  const uint16_t lab = selected ? theme::kBg : (item.enabled ? theme::kDim : theme::kDimmer);
  const uint16_t rv = selected ? val : (item.enabled ? val : theme::kDimmer);
  const int midY = r.y + r.h / 2;

  const lgfx::IFont* labelFont = font;
  const lgfx::IFont* valueFont = font;
  if (item.visual == RowVisual::Text && item.value && item.value[0]) {
    const int avail = r.w - 16 - 6;
    if (g.textWidth(item.label, labelFont) + g.textWidth(item.value, valueFont) > avail) {
      valueFont = typeface::micro();
      if (g.textWidth(item.label, labelFont) + g.textWidth(item.value, valueFont) > avail) {
        labelFont = typeface::micro();
      }
    }
  }
  g.str(item.label, r.x + 8, midY, lab, labelFont, textdatum_t::middle_left);
  switch (item.visual) {
    case RowVisual::Toggle:
      toggle(g, r.x + r.w - 8, midY, item.level != 0, selected ? theme::kBg : theme::kHi);
      break;
    case RowVisual::Gauge:
      gauge(g, r.x + r.w - 8, midY, std::min(r.w / 3, 60), item.level,
            selected ? theme::kBg : theme::kHi);
      break;
    case RowVisual::Text:
      if (item.value && item.value[0]) {
        g.str(item.value, r.x + r.w - 8, midY, rv, valueFont, textdatum_t::middle_right);
      }
      break;
  }
}

void listView(Gfx& g, const Layout& L, const ListItem* items, int count, int sel) {
  const lgfx::IFont* font = uiFont(L.landscape);
  const int rowH = L.landscape ? 18 : 22;
  const int top = L.top + 26;

  if (L.landscape) {
    const int perCol = (count + 1) / 2;
    const int colW = L.w / 2 - 8;
    for (int i = 0; i < count; ++i) {
      const int x = 4 + (i / perCol) * (L.w / 2);
      const int y = top + (i % perCol) * rowH;
      listRow(g, {x, y - 2, colW, rowH - 4}, items[i], i == sel, font);
    }
  } else {
    const int rowW = L.w - 8;
    const int visible = std::max(1, (L.bottom - top) / rowH);
    const int first = sel < visible ? 0 : sel - visible + 1;
    for (int i = first; i < count && i - first < visible; ++i) {
      listRow(g, {4, top + (i - first) * rowH - 2, rowW, rowH - 2}, items[i], i == sel, font);
    }
  }
}

void infoList(Gfx& g, const Layout& L, int top, const ListItem* items, int count) {
  if (count <= 0) return;
  auto& c = g.c();
  const int avail = L.bottom - top;
  const int rowH = avail / count;
  for (int i = 0; i < count; ++i) {
    const int y = top + i * rowH;
    const int mid = y + rowH / 2;
    if (i > 0) c.drawFastHLine(12, y, L.w - 24, theme::kDimmer);
    g.str(items[i].label, 12, mid, theme::kDim, typeface::micro(), textdatum_t::middle_left);
    const char* v = items[i].value ? items[i].value : "";
    const int keyW = g.textWidth(items[i].label, typeface::micro());
    const int maxV = L.w - 24 - keyW - 8;
    g.str(v, L.w - 12, mid, theme::kFg, fitFont(g, v, maxV, typeface::body(), typeface::micro()),
          textdatum_t::middle_right);
  }
}

void confirmPrompt(Gfx& g, const Layout& L, const char* title, const char* sub) {
  g.str(title, L.cx, L.cy - 10, theme::kHi, typeface::body(), textdatum_t::middle_center);
  if (sub && sub[0]) {
    g.str(sub, L.cx, L.cy + 12, theme::kDim, typeface::micro(), textdatum_t::middle_center);
  }
}

void trend(Gfx& g, int cx, int cy, char dir) {
  if (dir != 'u' && dir != 'd') return;
  const bool up = dir == 'u';
  const uint16_t col = up ? theme::kHi : theme::kCrit;
  const int s = 5;
  auto& c = g.c();
  if (up) {
    c.fillTriangle(cx - s, cy + s, cx + s, cy + s, cx, cy - s, col);
  } else {
    c.fillTriangle(cx - s, cy - s, cx + s, cy - s, cx, cy + s, col);
  }
}

void pill(Gfx& g, int cx, int y, const char* text, const lgfx::IFont* font, uint16_t color) {
  auto& c = g.c();
  c.setFont(font);
  const int tw = c.textWidth(text);
  const int th = c.fontHeight();
  const int w = tw + 18;
  const int h = th + 6;
  c.drawRoundRect(cx - w / 2, y, w, h, h / 2, theme::kDim);
  g.str(text, cx, y + h / 2, color, font, textdatum_t::middle_center);
}

void heroValue(Gfx& g, int cx, int cy, const char* label, const char* value,
               const lgfx::IFont* valueFont, int labelGap) {
  g.c().setFont(valueFont);
  const int vh = g.c().fontHeight();
  if (label && label[0]) {
    g.str(label, cx, cy - vh / 2 - labelGap, theme::kDim, typeface::body(),
          textdatum_t::bottom_center);
  }
  g.str(value, cx, cy, theme::kFg, valueFont, textdatum_t::middle_center);
}

const lgfx::IFont* heroFont(Gfx& g, const char* value, int maxWidth) {
  return fitFont(g, value, maxWidth, typeface::display(), typeface::title(), typeface::body());
}

void statBarAt(Gfx& g, int x, int y, int width, const char* label, int pct,
               const StatStyle& s) {
  const int clamped = std::max(0, std::min(100, pct));
  auto& c = g.c();
  g.str(label, x, y, theme::kDim, s.font, textdatum_t::top_left);

  const int bx = s.labelW > 0 ? x + s.labelW : x;
  const int by = s.labelW > 0 ? y - 1 : y + 15;
  const int bw = s.labelW > 0 ? width - s.labelW : width;
  const int fillW = ((bw - 2) * clamped) / 100;
  const uint16_t fill =
      (s.critThreshold > 0 && clamped < s.critThreshold) ? theme::kCrit : theme::kFg;

  if (s.rounded) {
    c.drawRoundRect(bx, by, bw, s.barH, 3, theme::kDim);
    if (fillW > 0) c.fillRoundRect(bx + 1, by + 1, fillW, s.barH - 2, 2, fill);
  } else {
    c.drawRect(bx, by, bw, s.barH, theme::kDim);
    c.fillRect(bx + 1, by + 1, fillW, s.barH - 2, fill);
  }
}

void statBar(Gfx& g, int y, const char* label, int pct) {
  statBarAt(g, 8, y, g.w() - 16, label, pct);
}

void dots(Gfx& g, int cx, int y, int count, int active) {
  if (count <= 1) return;
  const int gap = 8;
  const int startX = cx - (count - 1) * gap / 2;
  for (int i = 0; i < count; ++i) {
    const int x = startX + i * gap;
    if (i == active) {
      g.c().fillCircle(x, y, 2, theme::kFg);
    } else {
      g.c().drawCircle(x, y, 2, theme::kDim);
    }
  }
}

}  // namespace tama::widgets
