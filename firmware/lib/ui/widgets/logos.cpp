#include "logos.h"

#include <algorithm>

#include "logo.gen.h"

namespace tama::logos {

namespace {

struct Runtime {
  bool set = false;
  std::string id;
  int w = 0;
  int h = 0;
  std::vector<uint8_t> bits;

  bool at(int x, int y) const {
    const int i = y * w + x;
    return (bits[i >> 3] >> (7 - (i & 7))) & 1;
  }
} g_rt;

void drawRuntime(Gfx& g, int cx, int cy, int size, uint16_t col) {
  const int h = std::max(1, size);
  const int w = std::max(1, g_rt.w * h / g_rt.h);
  const int ox = cx - w / 2;
  const int oy = cy - h / 2;
  for (int dy = 0; dy < h; ++dy) {
    const int sy = dy * g_rt.h / h;
    for (int dx = 0; dx < w; ++dx) {
      const int sx = dx * g_rt.w / w;
      if (g_rt.at(sx, sy)) g.c().drawPixel(ox + dx, oy + dy, col);
    }
  }
}

}  // namespace

void setRuntime(const std::string& id, int w, int h, const std::vector<uint8_t>& bits) {
  const size_t need = static_cast<size_t>((w * h + 7) / 8);
  g_rt.set = w > 0 && h > 0 && !id.empty() && bits.size() >= need;
  if (!g_rt.set) return;
  g_rt.id = id;
  g_rt.w = w;
  g_rt.h = h;
  g_rt.bits = bits;
}

bool has(const std::string& id) {
  if (g_rt.set && id == g_rt.id) return true;
#if TAMA_HAS_LOGO
  return id == kBrandLogoId;
#else
  (void)id;
  return false;
#endif
}

int width(int height) {
  if (height <= 0) return 0;
  if (g_rt.set) return g_rt.w * height / g_rt.h;
#if TAMA_HAS_LOGO
  return kLogoW * height / kLogoH;
#else
  return 0;
#endif
}

bool draw(Gfx& g, const std::string& id, int cx, int cy, int size, uint16_t col) {
  if (g_rt.set && id == g_rt.id) {
    drawRuntime(g, cx, cy, size, col);
    return true;
  }
#if TAMA_HAS_LOGO
  if (id == kBrandLogoId) {
    drawBrandLogo(g, cx, cy, size, col);
    return true;
  }
#else
  (void)g;
  (void)id;
  (void)cx;
  (void)cy;
  (void)size;
  (void)col;
#endif
  return false;
}

}  // namespace tama::logos
