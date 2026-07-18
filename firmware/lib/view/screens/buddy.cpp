#include <cmath>
#include <cstdio>
#include <string>

#include "mascot.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

Expr exprForPhase(BuddyPhase phase) {
  switch (phase) {
    case BuddyPhase::Working: return Expr::Think;
    case BuddyPhase::Waiting: return Expr::Alert;
    case BuddyPhase::Done: return Expr::Celebrate;
    case BuddyPhase::Idle: return Expr::Happy;
    case BuddyPhase::Offline:
    default: return Expr::Sleepy;
  }
}

std::string humanTokens(uint64_t v) {
  char buf[24];
  if (v >= 1000000) {
    std::snprintf(buf, sizeof(buf), "%.1fM", static_cast<double>(v) / 1e6);
  } else if (v >= 1000) {
    std::snprintf(buf, sizeof(buf), "%.1fk", static_cast<double>(v) / 1e3);
  } else {
    std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(v));
  }
  return buf;
}

class BuddyScreen : public AppScreen {
 public:
  const char* id() const override { return "buddy"; }

  void render(Gfx& g, ShellContext& ctx) override {
    const BuddyState& b = ctx.state.buddy;
    const auto L = widgets::frame(g, ctx.state, "BUDDY");

    const int size = L.landscape ? 44 : 52;
    const int mascotY = L.landscape ? L.cy - 8 : L.top + 32 + size / 2;
    const int mascotX = L.landscape ? widgets::anchor(L, widgets::Side::Left) : L.cx;
    if (ctx.character) {
      MascotState m{exprForPhase(b.phase)};
      m.wanderPx = b.phase == BuddyPhase::Working ? 8 : 0;
      ctx.character->draw(g, mascotX, mascotY, size, m, now_);
    }

    if (b.phase == BuddyPhase::Done) {
      if (prevPhase_ != BuddyPhase::Done) burstMs_ = now_;
      drawConfetti(g, mascotX, mascotY, size, now_ - burstMs_);
    }
    prevPhase_ = b.phase;

    const int col = L.landscape ? L.w / 2 + 4 : 4;
    const int colW = L.landscape ? L.w - col - 6 : L.w - 8;
    const int colCx = col + colW / 2;

    int y = L.landscape ? L.top + 20 : widgets::mascotNameY(mascotY, size);
    g.str(buddyPhaseLabel(b.phase), colCx, y, theme::kHi, typeface::body(), textdatum_t::top_center);
    y += 16;

    const bool hasTokens = b.tokens > 0 || b.tokens_today > 0;
    const int maxY = L.bottom - (hasTokens ? 16 : 2);

    if (b.phase == BuddyPhase::Offline || b.phase == BuddyPhase::Idle) {
      renderQuiet(g, b, colCx, colW, y);
    } else {
      renderActive(g, b, colCx, col, colW, y, maxY);
    }

    if (hasTokens) {
      const std::string tok = "tok " + humanTokens(b.tokens) + "  today " + humanTokens(b.tokens_today);
      g.str(tok.c_str(), L.cx, L.bottom - 8, theme::kDim, typeface::micro(),
            textdatum_t::bottom_center);
    }

    widgets::hints(g, nullptr, "BACK");
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Next) return Transition::back();
    return Transition::none();
  }

  Transition tick(ShellContext&, uint32_t nowMs) override {
    now_ = nowMs;
    return anim_.due(nowMs, 60) ? Transition::redraw() : Transition::none();
  }

 private:
  void renderQuiet(Gfx& g, const BuddyState& b, int cx, int colW, int y) {
    if (!b.owner.empty()) {
      const std::string hi = "hi " + b.owner;
      g.str(hi.c_str(), cx, y, theme::kFg, typeface::body(), textdatum_t::top_center);
      y += 18;
    }
    const char* line = b.phase == BuddyPhase::Offline ? "start Claude and I'll wake up"
                                                      : "waiting for something to do";
    widgets::wrapText(g, line, cx, y, colW, typeface::micro(), theme::kDim, 11);
  }

  void renderActive(Gfx& g, const BuddyState& b, int cx, int colX, int colW, int y, int maxY) {
    if (!b.msg.empty()) {
      g.str(b.msg.c_str(), cx, y, theme::kFg,
            widgets::fitFont(g, b.msg.c_str(), colW, typeface::body(), typeface::micro()),
            textdatum_t::top_center);
      y += 16;
    }
    for (const auto& entry : b.entries) {
      if (y + 11 > maxY) break;
      const char* text = entry.c_str();
      const lgfx::IFont* font = widgets::fitFont(g, text, colW, typeface::micro());
      g.str(text, colX, y, theme::kDim, font, textdatum_t::top_left);
      y += 11;
    }
  }

  void drawConfetti(Gfx& g, int cx, int cy, int size, uint32_t elapsed) {
    constexpr uint32_t kLifeMs = 1600;
    if (elapsed > kLifeMs) return;
    const float t = elapsed / 1000.0f;
    const uint16_t pal[4] = {theme::kHi, theme::kWarn, theme::kBlush, theme::kFg};
    const int px = std::max(2, size / 22);
    auto& c = g.c();
    for (int i = 0; i < 20; ++i) {
      const uint32_t h = hash(static_cast<uint32_t>(i) + 1u);
      const float ang = static_cast<float>(h % 360u) * 0.017453293f;
      const float spd = 46.0f + static_cast<float>((h >> 4) % 30u);
      const int x = cx + static_cast<int>(std::lround(std::cos(ang) * spd * t));
      const int y = cy - size / 2 + static_cast<int>(std::lround(std::sin(ang) * spd * t + 70.0f * t * t));
      c.fillRect(x, y, px, px, pal[i & 3]);
    }
  }

  static uint32_t hash(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
  }

  uint32_t now_ = 0;
  BuddyPhase prevPhase_ = BuddyPhase::Idle;
  uint32_t burstMs_ = 0;
  AnimClock anim_;
};

}  // namespace

AppScreen& buddy() {
  static BuddyScreen instance;
  return instance;
}

}  // namespace tama::screens
