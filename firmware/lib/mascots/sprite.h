#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "character.h"
#include "gfx.h"
#include "mascot.h"
#include "theme.h"

namespace tama {

struct SpriteDef {
  const char* id;
  const char* name;
  const char* category;
  int w;
  int h;
  const uint16_t* palette;
  const uint8_t* const* frames;
  int fOpen;
  int fBlink;
  int fHappy;
  int fSleepy;
  int fWorried;
  int fAlert;
  bool blinks;
  bool wheeled;
  int outlineIndex;
};

class SpriteChar : public Character {
 public:
  explicit SpriteChar(const SpriteDef& def) : d_(def) {
    uint32_t h = 2166136261u;
    for (const char* p = def.id; *p; ++p) {
      h ^= static_cast<uint8_t>(*p);
      h *= 16777619u;
    }
    seed_ = h;
  }

  const char* id() const override { return d_.id; }
  const char* name() const override { return d_.name; }
  const char* category() const override { return d_.category; }

  void draw(Gfx& g, int cx, int cy, int size, const MascotState& st, uint32_t tickMs) override {
    const int base = size / 22 < 1 ? 1 : size / 22;
    const Motion m = computeMotion(st, tickMs);

    const int groundY = cy + (d_.h * base) / 2 - 1;
    if (st.shadow) drawShadow(g, cx + m.dx, groundY, d_.w * base, base, m.hop + st.liftPx / 12);

    const int frame = pickFrame(st.expr, tickMs);
    const int ox = cx - (d_.w * base) / 2 + m.dx;
    const int oy = cy - (d_.h * base) / 2 + m.dy - st.liftPx;
    blitFrame(g, ox, oy, base, frame, m.facing);

    if (st.expr == Expr::Sleepy) {
      drawZzz(g, cx + (d_.w * base) / 3, cy - (d_.h * base) / 2, tickMs * 0.001f);
    }
  }

 private:
  enum class Action { Idle, Walk, Hop, Turn };

  struct Motion {
    int dx = 0;
    int dy = 0;
    int facing = 1;
    int hop = 0;
  };

  Motion computeMotion(const MascotState& st, uint32_t tickMs) const {
    Motion m;
    if (st.still) return m;

    const float t = tickMs * 0.001f;
    const bool lively = st.expr == Expr::Neutral || st.expr == Expr::Think ||
                        st.expr == Expr::Happy || st.expr == Expr::Celebrate;
    const bool party = st.expr == Expr::Happy || st.expr == Expr::Celebrate;
    const bool wheeled = d_.wheeled;

    m.dy = wheeled ? static_cast<int>(std::lround(std::sin(t * 8.5f) * 0.7f))
                   : static_cast<int>(std::lround(std::sin(t * 2.2f) * 1.2f));

    if (lively) {
      const float beat = party ? 2.6f : 3.6f;
      const int idx = static_cast<int>(std::floor(t / beat));
      const float ph = t / beat - idx;
      const uint32_t roll = hash(static_cast<uint32_t>(idx) + seed_) % (party ? 6u : 8u);
      const Action a = party ? partyAction(roll) : idleAction(roll);

      if (a == Action::Walk && st.wanderPx > 0) {
        const float w = ph * 6.2831853f;
        m.dx = static_cast<int>(std::lround(std::sin(w) * st.wanderPx));
        m.facing = std::cos(w) >= 0.f ? 1 : -1;
        m.dy -= static_cast<int>(std::lround(std::fabs(std::sin(w * 2.f)) * 1.0f));
      } else if (a == Action::Hop) {
        if (wheeled) {
          const float win = ph * beat / 0.5f;
          if (win <= 1.f)
            m.dy -= static_cast<int>(std::lround(std::fabs(std::sin(win * 9.4f)) * 1.5f));
        } else {
          const float win = ph * beat / 0.45f;
          if (win <= 1.f) {
            const float up = 4.f * win * (1.f - win);
            m.hop = static_cast<int>(std::lround(up * (party ? 6.f : 5.f)));
          }
        }
      } else if (a == Action::Turn) {
        m.facing = ph < 0.5f ? 1 : -1;
      }
    } else if (st.expr == Expr::Alert) {
      m.dx = static_cast<int>(std::lround(std::sin(t * 34.f) * 1.5f));
    } else if (st.expr == Expr::Worried &&
               (hash(static_cast<uint32_t>(t * 2.f) + seed_) & 15u) == 0u) {
      m.dx = 1;
    } else if (st.expr == Expr::Sleepy) {
      m.dy = static_cast<int>(std::lround(std::sin(t * 1.3f) * 1.0f));
    }

    m.dy -= m.hop;
    return m;
  }

  void blitFrame(Gfx& g, int ox, int oy, int base, int frame, int facing) const {
    auto& c = g.c();
    const uint8_t* fr = d_.frames[frame];
    for (int y = 0; y < d_.h; ++y) {
      for (int x = 0; x < d_.w; ++x) {
        const int srcx = facing > 0 ? x : d_.w - 1 - x;
        const uint8_t idx = fr[y * d_.w + srcx];
        if (!idx) continue;
        const uint16_t col = idx == d_.outlineIndex ? theme::kInk : d_.palette[idx];
        c.fillRect(ox + x * base, oy + y * base, base, base, col);
      }
    }
  }

  static Action idleAction(uint32_t roll) {
    switch (roll) {
      case 5: return Action::Walk;
      case 6: return Action::Hop;
      case 7: return Action::Turn;
      default: return Action::Idle;
    }
  }

  static Action partyAction(uint32_t roll) {
    switch (roll) {
      case 2: return Action::Walk;
      case 3:
      case 4: return Action::Hop;
      case 5: return Action::Turn;
      default: return Action::Idle;
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

  void drawShadow(Gfx& g, int cx, int groundY, int spriteW, int base, int hop) const {
    const int rx = std::max(3, spriteW / 4 - hop);
    const int ry = std::max(1, base - hop / 4);
    g.c().fillEllipse(cx, groundY + 2, rx, ry, theme::kDimmer);
  }

  void drawZzz(Gfx& g, int x, int y, float t) const {
    float a = t * 0.6f;
    a -= std::floor(a);
    float b = a + 0.5f;
    b -= std::floor(b);
    g.str("z", x, y - static_cast<int>(std::lround(a * 12.f)), theme::kDim, typeface::body());
    g.str("z", x + 6, y + 4 - static_cast<int>(std::lround(b * 12.f)), theme::kDimmer,
          typeface::body());
  }

  int pickFrame(Expr expr, uint32_t tickMs) const {
    switch (expr) {
      case Expr::Happy:
      case Expr::Celebrate: return d_.fHappy;
      case Expr::Sleepy: return d_.fSleepy;
      case Expr::Worried: return d_.fWorried;
      case Expr::Alert: return d_.fAlert;
      case Expr::Think:
      case Expr::Neutral:
      default: return d_.blinks && (tickMs % 3400u) < 160u ? d_.fBlink : d_.fOpen;
    }
  }

  const SpriteDef& d_;
  uint32_t seed_ = 0;
};

}  // namespace tama
