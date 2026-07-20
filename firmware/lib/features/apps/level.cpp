#include "brand.gen.h"
#if TAMA_APP_LEVEL

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "apps.h"
#include "motion.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr float kPi = 3.14159265f;
constexpr float kTolDeg = 1.0f;
constexpr float kRangeDeg = 12.0f;
constexpr float kSmoothing = 0.18f;
constexpr float kFlatZ = 0.80f;

float toDeg(float rad) { return rad * 180.0f / kPi; }

float axisDeg(float component) {
  return toDeg(std::asin(std::clamp(component, -1.0f, 1.0f)));
}

void degValue(Gfx& g, int cx, int cy, const char* text, const lgfx::IFont* font, uint16_t color) {
  g.str(text, cx, cy, color, font, textdatum_t::middle_center);
  g.c().setFont(font);
  const int tw = g.c().textWidth(text);
  const int th = g.c().fontHeight();
  g.c().drawCircle(cx + tw / 2 + 4, cy - th / 2 + 3, 2, color);
}

class LevelScreen : public AppScreen {
 public:
  const char* id() const override { return "app.level"; }

  void onEnter(ShellContext& ctx) override {
    sensor_ = &ctx.sensor;
    filter_.reset();
    zeroed_ = false;
    pitch0_ = roll0_ = edge0_ = 0.0f;
    sample(true);
  }

  void onExit() override { sensor_ = nullptr; }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "LEVEL");

    float sx = filter_.x(), sy = filter_.y();
    if (g.rotation() == 1) {
      sx = filter_.y();
      sy = -filter_.x();
    }

    if (flat_) {
      renderFlat(g, L, sx, sy);
    } else {
      renderEdge(g, L, sx, sy);
    }

    widgets::hints(g, zeroed_ ? "RESET" : "ZERO", "BACK");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    if (intent != Intent::Select) return Transition::none();
    if (zeroed_) {
      zeroed_ = false;
      pitch0_ = roll0_ = edge0_ = 0.0f;
    } else {
      zeroed_ = true;
      pitch0_ = axisDeg(filter_.y());
      roll0_ = axisDeg(filter_.x());
      edge0_ = edgeDeviation(filter_.x(), filter_.y(), nullptr);
    }
    return Transition::redraw();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    if (!anim_.due(nowMs, 33)) return Transition::none();
    sample(false);
    return Transition::redraw();
  }

 private:
  void sample(bool reset) {
    if (!filter_.sample(sensor_, reset)) return;
    flat_ = std::fabs(filter_.z()) >= kFlatZ;
  }

  // Angle of gravity in the screen plane, folded to the nearest rest
  // orientation. snapUpright says whether that rest is plumb (long axis
  // vertical) or level (long axis horizontal).
  static float edgeDeviation(float sx, float sy, bool* snapUpright) {
    const float theta = toDeg(std::atan2(sx, sy));
    const float snap = std::round(theta / 90.0f) * 90.0f;
    if (snapUpright) *snapUpright = std::fmod(std::fabs(snap), 180.0f) < 45.0f;
    return theta - snap;
  }

  void renderFlat(Gfx& g, const widgets::Layout& L, float sx, float sy) {
    const float pitch = axisDeg(sy) - (zeroed_ ? pitch0_ : 0.0f);
    const float roll = axisDeg(sx) - (zeroed_ ? roll0_ : 0.0f);
    const bool level = std::fabs(pitch) <= kTolDeg && std::fabs(roll) <= kTolDeg;
    const uint16_t accent = level ? theme::kHi : theme::kWarn;

    const int cx = L.landscape ? L.w / 3 - 8 : L.cx;
    const int contentTop = L.top + 14;
    const int r = L.landscape ? (L.bottom - contentTop) / 2 - 6
                              : std::min((L.w - 28) / 2, (L.contentH() - 64) / 2);
    const int cy = L.landscape ? (contentTop + L.bottom) / 2 : contentTop + r;

    auto& c = g.c();
    c.drawFastHLine(cx - r - 8, cy, 2 * r + 16, theme::kDimmer);
    c.drawFastVLine(cx, cy - r - 8, 2 * r + 16, theme::kDimmer);
    c.drawCircle(cx, cy, r, theme::kDim);
    c.drawCircle(cx, cy, r / 2, theme::kDimmer);

    const int rTol = std::max(6, static_cast<int>(r * kTolDeg / kRangeDeg) + 5);
    c.drawCircle(cx, cy, rTol, level ? accent : theme::kDim);

    const float scale = r / kRangeDeg;
    const int bx = cx - static_cast<int>(std::clamp(roll * scale, -(float)r, (float)r));
    const int by = cy - static_cast<int>(std::clamp(pitch * scale, -(float)r, (float)r));
    c.fillCircle(bx, by, 5, accent);
    c.drawCircle(bx, by, 5, theme::kFg);
    c.drawPixel(bx - 2, by - 2, theme::kFg);

    char pv[8], rv[8];
    std::snprintf(pv, sizeof(pv), "%+.1f", pitch);
    std::snprintf(rv, sizeof(rv), "%+.1f", roll);

    if (L.landscape) {
      const int colX = 2 * L.w / 3 + 8;
      readout(g, colX - 34, cy - 14, "PITCH", pv);
      readout(g, colX + 34, cy - 14, "ROLL", rv);
      widgets::pill(g, colX, cy + 18, level ? "LEVEL" : "TILTED", typeface::micro(), accent);
    } else {
      const int rowY = cy + r + 20;
      readout(g, L.cx - 32, rowY, "PITCH", pv);
      readout(g, L.cx + 32, rowY, "ROLL", rv);
      widgets::pill(g, L.cx, rowY + 30, level ? "LEVEL" : "TILTED", typeface::micro(), accent);
    }
  }

  void renderEdge(Gfx& g, const widgets::Layout& L, float sx, float sy) {
    bool upright = false;
    const float dev = edgeDeviation(sx, sy, &upright) - (zeroed_ ? edge0_ : 0.0f);
    const bool level = std::fabs(dev) <= kTolDeg;
    const uint16_t accent = level ? theme::kHi : theme::kWarn;

    auto& c = g.c();
    const int trackW = L.w - 28;
    const int trackH = 22;
    const int tx = L.cx - trackW / 2;
    const int ty = L.top + (L.landscape ? 16 : 34);
    const int tcy = ty + trackH / 2;

    c.drawRoundRect(tx, ty, trackW, trackH, trackH / 2, theme::kDim);
    const int gap = std::max(8, static_cast<int>((trackW / 2) * kTolDeg / kRangeDeg) + 6);
    c.drawFastVLine(L.cx - gap, ty - 4, trackH + 8, level ? accent : theme::kDim);
    c.drawFastVLine(L.cx + gap, ty - 4, trackH + 8, level ? accent : theme::kDim);

    const float scale = (trackW / 2 - 12) / kRangeDeg;
    const int bx = L.cx - static_cast<int>(std::clamp(dev * scale, -(float)(trackW / 2 - 12),
                                                      (float)(trackW / 2 - 12)));
    c.fillCircle(bx, tcy, 7, accent);
    c.drawCircle(bx, tcy, 7, theme::kFg);
    c.drawPixel(bx - 2, tcy - 3, theme::kFg);

    char dv[8];
    std::snprintf(dv, sizeof(dv), "%+.1f", dev);
    const int heroY = tcy + (L.landscape ? 34 : 52);
    degValue(g, L.cx, heroY, dv, widgets::heroFont(g, "+88.8", L.w - 40), theme::kFg);

    const int pillY = heroY + (L.landscape ? 22 : 34);
    widgets::pill(g, L.cx, pillY, level ? (upright ? "PLUMB" : "LEVEL") : "TILTED",
                  typeface::micro(), accent);
  }

  void readout(Gfx& g, int cx, int y, const char* label, const char* value) {
    g.str(label, cx, y, theme::kDim, typeface::micro(), textdatum_t::top_center);
    degValue(g, cx, y + 20, value, typeface::title(), theme::kFg);
  }

  AnimClock anim_;
  ISensorSource* sensor_ = nullptr;
  motion::TiltFilter filter_{kSmoothing};
  bool flat_ = true;
  bool zeroed_ = false;
  float pitch0_ = 0.0f;
  float roll0_ = 0.0f;
  float edge0_ = 0.0f;
};

}  // namespace

TAMA_SCREEN_FACTORY(level, LevelScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_LEVEL
