// Copyright (c) Meta Platforms, Inc. and affiliates.
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Ported from esp32/avatar/muse_pixel.c in the Muse gadget SDK. Modified: C++,
// drawn on a transparent background without the glow, rings, waves, sparkles,
// hearts, floor shadow, rim light, ordered dithering or grid scaler.

#include "jolly.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"

namespace tama {

namespace {

constexpr int W = 64;
constexpr int H = 64;
constexpr float TAU = 6.2831853f;

enum class Mode { Idle, Listening, Thinking, Speaking, Error, Off, Count };

struct Pose {
  Mode mode;
  float t;
  float mode_t;
  float level;
  float happy;
};

enum {
  C_BG = 0,
  C_OUT,
  C_OUT2,
  C_BD,
  C_BM,
  C_BL,
  C_BH,
  C_SKIND,
  C_SKIN,
  C_SKINL,
  C_IRIS,
  C_SHINE,
  C_BROW,
  C_BLUSH,
  C_BLUSHD,
  C_MOUTH,
  C_TONGUE,
  C_G0,
  C_G1,
  C_G2,
  C_ACC,
  C_COUNT,
};

struct rgb_t {
  float r, g, b;
};

struct scheme_t {
  uint32_t f[3];
  uint32_t acc;
};

const scheme_t SCHEMES[static_cast<int>(Mode::Count)] = {
    {{0xf4e8ff, 0xc7a4ff, 0x9a6bff}, 0xa77dff},
    {{0xe8faff, 0x8fdcff, 0x3fa2ff}, 0x5cb8ff},
    {{0xffe6ff, 0xff9cf0, 0xd35bff}, 0xe07bff},
    {{0xeafff4, 0x9ff5cf, 0x3fd9a0}, 0x6ff0bf},
    {{0xffd6d6, 0xff6b6b, 0xc7304a}, 0xff5c5c},
    {{0xd8d4ff, 0x8f86d9, 0x5a4fb0}, 0x7c72d0},
};

const uint32_t FIXED[C_COUNT] = {
    0x000000, 0x3a2b22, 0x8c7560, 0xae987e, 0xcfbc9f, 0xe6d7bd, 0xf8eedc, 0xe9cba4,
    0xf6dfbd, 0xfdeed6, 0x120d0b, 0xffffff, 0x6b5444, 0xf4aaa0, 0xea8f8e, 0x3a1f1a,
    0xe86a7a, 0,        0,        0,        0,
};

rgb_t s_scheme[4];
bool s_scheme_init;
uint16_t s_pal[C_COUNT];
uint8_t s_fb[W * H];
uint8_t s_mask[W * H];

// Fixed point Q12 with lookup tables: chips without an FPU emulate float in
// software, which made a frame take 250 ms.
constexpr int Q = 12;
constexpr int32_t ONE = 1 << Q;
constexpr int32_t QF(float v) { return static_cast<int32_t>(v * ONE); }
constexpr int POW_LUT_N = 256;
constexpr int32_t POW_LUT_MAX_Q = QF(1.2f);
constexpr int SQRT_LUT_N = 1024;
int16_t s_pow_dome[POW_LUT_N + 1];
int16_t s_pow_base[POW_LUT_N + 1];
int16_t s_sqrt[SQRT_LUT_N + 1];

void init_luts() {
  for (int i = 0; i <= POW_LUT_N; i++) {
    float u = static_cast<float>(i) * POW_LUT_MAX_Q / POW_LUT_N / ONE;
    s_pow_dome[i] = static_cast<int16_t>(powf(u, 2.7f) * ONE);
    s_pow_base[i] = static_cast<int16_t>(powf(u, 3.6f) * ONE);
  }
  for (int i = 0; i <= SQRT_LUT_N; i++) {
    s_sqrt[i] = static_cast<int16_t>(sqrtf(static_cast<float>(i) / SQRT_LUT_N) * ONE);
  }
}

inline int32_t pow_q(const int16_t* lut, int32_t a) {
  return lut[(a * (POW_LUT_N * 65536 / POW_LUT_MAX_Q)) >> 16];
}

inline int32_t sqrt_q(int32_t x) { return s_sqrt[x >> 2]; }

inline rgb_t hex_rgb(uint32_t c) {
  return {static_cast<float>((c >> 16) & 0xff), static_cast<float>((c >> 8) & 0xff),
          static_cast<float>(c & 0xff)};
}

inline rgb_t mix(rgb_t a, rgb_t b, float t) {
  return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

inline uint16_t to565(rgb_t c) {
  int r = static_cast<int>(c.r + 0.5f), g = static_cast<int>(c.g + 0.5f),
      b = static_cast<int>(c.b + 0.5f);
  r = r < 0 ? 0 : (r > 255 ? 255 : r);
  g = g < 0 ? 0 : (g > 255 ? 255 : g);
  b = b < 0 ? 0 : (b > 255 ? 255 : b);
  return static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void update_palette(const scheme_t& target, float dt) {
  rgb_t tgt[4];
  for (int i = 0; i < 3; i++) tgt[i] = hex_rgb(target.f[i]);
  tgt[3] = hex_rgb(target.acc);

  float k = s_scheme_init ? 1.0f - expf(-dt * 7.0f) : 1.0f;
  for (int i = 0; i < 4; i++) s_scheme[i] = mix(s_scheme[i], tgt[i], k);
  s_scheme_init = true;

  for (int i = 0; i < C_COUNT; i++) s_pal[i] = to565(hex_rgb(FIXED[i]));
  s_pal[C_G0] = to565(s_scheme[0]);
  s_pal[C_G1] = to565(s_scheme[1]);
  s_pal[C_G2] = to565(s_scheme[2]);
  s_pal[C_ACC] = to565(s_scheme[3]);
}

inline void px(int x, int y, uint8_t c) {
  if (static_cast<unsigned>(x) < W && static_cast<unsigned>(y) < H) s_fb[y * W + x] = c;
}

inline int iround(float v) { return static_cast<int>(floorf(v + 0.5f)); }

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline float fracf(float v) { return v - floorf(v); }

uint32_t s_rng = 0x9e3779b9u;
float frand() {
  s_rng ^= s_rng << 13;
  s_rng ^= s_rng >> 17;
  s_rng ^= s_rng << 5;
  return static_cast<float>(s_rng & 0xffffff) / static_cast<float>(0x1000000);
}

void stamp(const char* const* rows, int nrows, int x0, int y0, uint8_t fill, uint8_t alt) {
  for (int r = 0; r < nrows; r++) {
    for (int c = 0; rows[r][c]; c++) {
      char ch = rows[r][c];
      if (ch == '#') {
        px(x0 + c, y0 + r, fill);
      } else if (ch == 'o') {
        px(x0 + c, y0 + r, alt);
      }
    }
  }
}

struct eyes_t {
  float next_blink;
  float blink_start;
  float next_gaze;
  float gx, gy;
  float tgx, tgy;
  float last_t;
};

eyes_t s_eyes = {1.5f, -10, 1.0f, 0, 0, 0, 0, 0};

float eyes_update(const Pose& p, float dt) {
  eyes_t* e = &s_eyes;

  if (p.t >= e->next_blink) {
    e->blink_start = p.t;
    e->next_blink = p.t + (frand() < 0.2f ? 0.28f : 2.2f + frand() * 3.0f);
  }

  if (p.t >= e->next_gaze) {
    e->next_gaze = p.t + 1.2f + frand() * 2.4f;
    if (frand() < 0.35f) {
      e->tgx = 0;
      e->tgy = 0;
    } else {
      e->tgx = frand() * 2 - 1;
      e->tgy = (frand() * 2 - 1) * 0.6f;
    }
  }

  float tgx = e->tgx, tgy = e->tgy;
  switch (p.mode) {
    case Mode::Listening:
      tgx = 0;
      tgy = 0.1f;
      break;
    case Mode::Thinking:
      tgx = 0.75f * sinf(p.mode_t * 1.3f) + 0.25f;
      tgy = -0.85f;
      break;
    case Mode::Speaking:
      tgx *= 0.3f;
      tgy = 0;
      break;
    default: break;
  }
  float k = 1.0f - expf(-dt * 14.0f);
  e->gx += (tgx - e->gx) * k;
  e->gy += (tgy - e->gy) * k;

  float bt = (p.t - e->blink_start) / 0.16f;
  if (bt < 0 || bt > 1) return 0;
  return 1.0f - fabsf(bt * 2 - 1);
}

inline int32_t hash16(int x, int y) {
  uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
  h = (h ^ (h >> 13)) * 1274126177u;
  return static_cast<int32_t>((h ^ (h >> 16)) & 0xffff);
}

struct avatar_t {
  float cx, cy;
  float a, b;
  float fx, fy;
  float fa, fb;
};

struct row_t {
  int32_t v;
  int32_t inv_a;
  const int16_t* lut;
  bool body;
  int32_t v_pow;
  int32_t fv, fv4;
};

inline void row_setup(const avatar_t& j, float y, row_t& r) {
  float v = (y - j.cy) / j.b;
  float a = j.a * (1 + 0.07f * clampf(v, -1, 1));
  r.v = QF(v);
  r.inv_a = QF(1.0f / a);
  r.lut = v < 0 ? s_pow_dome : s_pow_base;
  r.body = abs(r.v) <= POW_LUT_MAX_Q;
  r.v_pow = r.body ? pow_q(r.lut, abs(r.v)) : 0;
  float fv = clampf((y - j.fy) / j.fb, -8, 8);
  r.fv = QF(fv);
  r.fv4 = QF(fminf(fv * fv * fv * fv, 16));
}

inline int32_t body_field(const row_t& r, int32_t dx, int32_t* ux) {
  int32_t u = (dx * r.inv_a) >> Q;
  *ux = u;
  if (!r.body || abs(u) > POW_LUT_MAX_Q) return 2 * ONE;
  return pow_q(r.lut, abs(u)) + r.v_pow;
}

uint8_t fur(int32_t nx, int32_t ny, int x, int y, int ox, int oy) {
  int32_t r2 = (nx * nx + ny * ny) >> Q;
  int32_t nz = r2 >= ONE ? 0 : sqrt_q(ONE - r2);
  int32_t l = (-QF(0.40f) * nx - QF(0.50f) * ny + QF(0.76f) * nz) >> Q;
  int rx = x - ox, ry = y - oy;
  int32_t streak = (hash16(rx, (ry + (rx & 1) * 2) / 3) >> 4) - ONE / 2;
  int32_t lv = l + ((streak * QF(0.12f)) >> Q);
  return lv > QF(0.95f) ? C_BH : lv > QF(0.62f) ? C_BL : lv > QF(0.28f) ? C_BM : C_BD;
}

struct limb_t {
  float x, y;
  float angle;
};

struct limb_q_t {
  int32_t x, y, c, s, inv_rx, inv_ry, r;
};

void limb_setup(const limb_t& l, float rx, float ry, limb_q_t& q) {
  q.x = QF(l.x);
  q.y = QF(l.y);
  q.c = QF(cosf(l.angle));
  q.s = QF(sinf(l.angle));
  q.inv_rx = QF(1.0f / rx);
  q.inv_ry = QF(1.0f / ry);
  q.r = QF(rx > ry ? rx : ry);
}

inline bool in_limb(const limb_q_t& l, int32_t x, int32_t y, int32_t* lx, int32_t* ly) {
  int32_t dx = x - l.x, dy = y - l.y;
  if (abs(dx) > l.r || abs(dy) > l.r) return false;
  int32_t u = ((((dx * l.c) >> Q) + ((dy * l.s) >> Q)) * l.inv_rx) >> Q;
  int32_t v = ((((dy * l.c) >> Q) - ((dx * l.s) >> Q)) * l.inv_ry) >> Q;
  *lx = u;
  *ly = v;
  return u * u + v * v <= ONE * ONE;
}

enum { M_NONE, M_BODY, M_ARM, M_FOOT, M_FACE };

void draw_avatar(const avatar_t& j, const limb_t arms[2], const limb_t feet[2]) {
  memset(s_mask, 0, sizeof(s_mask));
  limb_q_t arm_q[2], foot_q[2];
  for (int i = 0; i < 2; i++) {
    limb_setup(arms[i], 2.9f, 5.2f, arm_q[i]);
    limb_setup(feet[i], 4.4f, 2.6f, foot_q[i]);
  }
  int x0 = static_cast<int>(j.cx - j.a * 1.1f - 8), x1 = static_cast<int>(j.cx + j.a * 1.1f + 8);
  int y0 = static_cast<int>(j.cy - j.b - 8), y1 = static_cast<int>(j.cy + j.b + 5);
  x0 = x0 < 0 ? 0 : x0;
  y0 = y0 < 0 ? 0 : y0;
  x1 = x1 >= W ? W - 1 : x1;
  y1 = y1 >= H ? H - 1 : y1;
  int ox = iround(j.cx), oy = iround(j.cy);
  int32_t cx = QF(j.cx), fcx = QF(j.fx), inv_fa = QF(1.0f / j.fa);

  for (int y = y0; y <= y1; y++) {
    row_t row;
    row_setup(j, y + 0.5f, row);
    int32_t fy = y * ONE + ONE / 2;
    for (int x = x0; x <= x1; x++) {
      int32_t fx = x * ONE + ONE / 2;
      int32_t lx, ly;

      bool arm = false;
      for (int a = 0; a < 2 && !arm; a++) {
        if (in_limb(arm_q[a], fx, fy, &lx, &ly)) {
          s_mask[y * W + x] = M_ARM;
          int32_t nx = ((lx * QF(0.85f)) >> Q) + (a ? QF(0.25f) : -QF(0.25f));
          px(x, y, fur(nx, (ly * QF(0.8f)) >> Q, x, y, ox, oy));
          arm = true;
        }
      }
      if (arm) continue;

      int32_t ux, uy = row.v;
      int32_t v = body_field(row, fx - cx, &ux);
      int32_t tuft = ((hash16(x - ox, y - oy) - 32768) * QF(0.16f)) >> 16;
      if (v <= ONE + tuft) {
        int32_t fu = ((fx - fcx) * inv_fa) >> Q;
        fu = fu > 3 * ONE ? 3 * ONE : fu < -3 * ONE ? -3 * ONE : fu;
        int32_t fu2 = (fu * fu) >> Q;
        int32_t ff = ((fu2 * fu2) >> Q) + row.fv4;
        if (ff <= ONE) {
          s_mask[y * W + x] = M_FACE;
          int32_t fv = row.fv, fv1 = fv + QF(0.15f);
          uint8_t c = C_SKIN;
          if (fv < -QF(0.73f) || fv > QF(0.85f)) {
            c = C_SKIND;
          } else if (((fu2 * QF(1.4f)) >> Q) + ((fv1 * fv1) >> Q) < QF(0.32f)) {
            c = C_SKINL;
          }
          px(x, y, c);
        } else {
          s_mask[y * W + x] = M_BODY;
          if (ff < QF(1.39f)) {
            px(x, y, ff < QF(1.3f) ? C_OUT2 : C_BD);
          } else {
            px(x, y, fur((ux * QF(0.95f)) >> Q, (uy * QF(0.95f)) >> Q, x, y, ox, oy));
          }
        }
        continue;
      }

      for (int f = 0; f < 2; f++) {
        if (in_limb(foot_q[f], fx, fy, &lx, &ly)) {
          s_mask[y * W + x] = M_FOOT;
          px(x, y, ly < -QF(0.2f) ? C_BM : C_BD);
          break;
        }
      }
    }
  }

  static const int8_t N4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      uint8_t m = s_mask[y * W + x];
      if (m == M_NONE || m == M_FACE) continue;
      for (int k = 0; k < 4; k++) {
        int xx = x + N4[k][0], yy = y + N4[k][1];
        uint8_t n = (static_cast<unsigned>(xx) < W && static_cast<unsigned>(yy) < H)
                        ? s_mask[yy * W + xx]
                        : static_cast<uint8_t>(M_NONE);
        if (n == M_NONE || (m == M_ARM && (n == M_BODY || n == M_FACE)) ||
            (m == M_BODY && n == M_FOOT)) {
          px(x, y, C_OUT);
          break;
        }
      }
    }
  }
}

enum eye_style_t { EYES_NORMAL, EYES_WIDE, EYES_HAPPY, EYES_X };

void draw_eye(float ex, float ey, float openness, eye_style_t style, float gx, float gy) {
  int cx = iround(ex + gx * 0.8f), cy = iround(ey + gy * 0.7f);

  if (style == EYES_HAPPY) {
    static const char* const HAPPY[] = {".##.", "#..#"};
    stamp(HAPPY, 2, iround(ex) - 2, iround(ey), C_IRIS, C_IRIS);
    return;
  }
  if (style == EYES_X) {
    static const char* const XS[] = {"#..#", ".##.", ".##.", "#..#"};
    stamp(XS, 4, iround(ex) - 2, iround(ey) - 1, C_IRIS, C_IRIS);
    return;
  }
  if (openness < 0.3f) {
    static const char* const SHUT[] = {"#..#", ".##."};
    stamp(SHUT, 2, iround(ex) - 2, iround(ey) + 1, C_IRIS, C_IRIS);
    return;
  }

  static const char* const BEAD[] = {".##.", "#o##", "####", ".##."};
  static const char* const BIG[] = {".##.", "#o##", "#o##", "####", ".##."};
  const char* const* rows = style == EYES_WIDE ? BIG : BEAD;
  int n = style == EYES_WIDE ? 5 : 4;
  int skip = iround((1 - openness) * (n - 1));
  stamp(rows + skip, n - skip, cx - 2, cy - 2 + skip, C_IRIS, skip ? C_IRIS : C_SHINE);
}

void draw_blush(int x, int y, float strength) {
  static const char* const CHEEK[] = {".##.", "####", ".##."};
  stamp(CHEEK, 3, x - 2, y, C_BLUSH, C_BLUSH);
  if (strength > 0.9f) {
    px(x - 1, y + 1, C_BLUSHD);
    px(x, y + 1, C_BLUSHD);
  }
}

enum mouth_t { MOUTH_SMILE, MOUTH_O, MOUTH_HMM, MOUTH_TALK, MOUTH_GRIN, MOUTH_FLAT };

void draw_mouth(int x, int y, mouth_t m, float open) {
  switch (m) {
    case MOUTH_SMILE: {
      static const char* const S[] = {"#..#", ".##."};
      stamp(S, 2, x - 2, y, C_MOUTH, C_MOUTH);
      break;
    }
    case MOUTH_O: {
      static const char* const S[] = {".##.", "#oo#", ".##."};
      stamp(S, 3, x - 2, y - 1, C_MOUTH, C_TONGUE);
      break;
    }
    case MOUTH_HMM: {
      static const char* const S[] = {"..#", "##."};
      stamp(S, 2, x - 1, y, C_MOUTH, C_MOUTH);
      break;
    }
    case MOUTH_TALK: {
      int h = 1 + iround(clampf(open, 0, 1) * 3.0f);
      int w = h > 2 ? 4 : 3;
      for (int j = 0; j < h; j++) {
        int inset = (j == 0 || j == h - 1) && h > 2 ? 1 : 0;
        for (int i = inset; i < w - inset; i++) {
          bool tongue = h >= 3 && j == h - 2 && i > inset && i < w - inset - 1;
          px(x - w / 2 + i, y + j, tongue ? C_TONGUE : C_MOUTH);
        }
      }
      break;
    }
    case MOUTH_GRIN: {
      static const char* const S[] = {"#####", ".#o#.", "..#.."};
      stamp(S, 3, x - 2, y, C_MOUTH, C_TONGUE);
      break;
    }
    case MOUTH_FLAT: {
      static const char* const S[] = {"##"};
      stamp(S, 1, x - 1, y + 1, C_MOUTH, C_MOUTH);
      break;
    }
  }
}

void draw_thought_dots(float x, float y, float t) {
  int active = static_cast<int>(fracf(t * 1.6f) * 3.0f);
  for (int i = 0; i < 3; i++) {
    int bx = iround(x + i * 4);
    int by = iround(y - i * 3) - (i == active ? 1 : 0);
    uint8_t c = i == active ? C_G0 : C_ACC;
    px(bx, by, c);
    px(bx + 1, by, c);
    px(bx, by + 1, c);
    px(bx + 1, by + 1, i == active ? C_G1 : C_G2);
  }
}

void draw_alert(int x, int y) {
  static const char* const BANG[] = {".##.", ".##.", ".##.", ".##.", "....", ".##."};
  stamp(BANG, 6, x - 2, y, C_ACC, C_ACC);
}

void render(const Pose& p) {
  static bool s_luts;
  if (!s_luts) {
    init_luts();
    s_luts = true;
  }
  float dt = s_eyes.last_t > 0 ? clampf(p.t - s_eyes.last_t, 0, 0.2f) : 0.04f;
  s_eyes.last_t = p.t;

  Mode mode = p.mode;
  float fade = mode == Mode::Off ? clampf(1.0f - p.mode_t / 1.3f, 0, 1) : 1.0f;
  float happy = p.happy;
  float level = p.level;
  float t = p.t;

  update_palette(SCHEMES[static_cast<int>(mode)], dt);
  float blink = eyes_update(p, dt);

  memset(s_fb, C_BG, sizeof(s_fb));

  float bob, breathe_rate = 2.0f, lean = 0, hop = 0;
  switch (mode) {
    case Mode::Listening: bob = sinf(t * 3.0f) * 0.6f; break;
    case Mode::Thinking:
      bob = sinf(t * 2.4f) * 0.8f;
      lean = sinf(t * 1.3f) * 1.2f;
      break;
    case Mode::Speaking: bob = sinf(t * 5.0f) * 0.6f - level * 1.5f; break;
    case Mode::Error:
      bob = 1.0f;
      lean = sinf(t * 18.0f) * (p.mode_t < 0.6f ? 1.0f : 0.0f);
      break;
    default: bob = sinf(t * 1.8f) * 1.0f; break;
  }
  if (happy > 0) hop = fabsf(sinf(t * 9.0f)) * 3.0f * happy;

  float breathe = sinf(t * breathe_rate + 1.0f) * 0.03f;
  avatar_t j;
  j.a = 16.0f * (1 + breathe) + level * 0.8f;
  j.b = 23.0f * (1 - breathe);
  j.cx = 32.0f + lean;
  j.cy = 56.5f - j.b + bob * 0.5f - hop;
  j.fa = j.a * 0.66f;
  j.fb = 7.4f;
  j.fx = j.cx + lean * 0.3f;
  j.fy = j.cy - j.b * 0.30f + bob * 0.3f;

  float base = j.cy + j.b;
  limb_t feet[2];
  float step = mode == Mode::Speaking ? sinf(t * 5.0f) * 0.6f : 0.0f;
  feet[0] = {j.cx - 7.0f, base - 0.5f + (happy > 0 ? hop * 0.3f : step), -0.15f};
  feet[1] = {j.cx + 7.0f, base - 0.5f + (happy > 0 ? hop * 0.3f : -step), 0.15f};

  limb_t arms[2];
  float adx = j.a + 0.3f;
  float ay = j.cy + 4.0f;
  switch (mode) {
    case Mode::Listening:
      arms[0] = {j.cx - adx + 1.0f, j.fy + 5.0f, 0.55f};
      arms[1] = {j.cx + adx - 1.0f, j.fy + 5.0f, -0.55f};
      break;
    case Mode::Thinking:
      arms[0] = {j.cx - adx, ay, -0.35f};
      arms[1] = {j.cx + 7.5f, j.fy + j.fb + 3.5f, -1.1f};
      break;
    case Mode::Off: {
      float w = sinf(t * 12.0f) * 0.35f * fade;
      arms[0] = {j.cx - adx, ay, -0.3f};
      arms[1] = {j.cx + adx + 1.0f, j.cy - 4.0f, -2.3f - w};
      break;
    }
    case Mode::Speaking: {
      float w = sinf(t * 7.0f) * (0.25f + level * 0.45f);
      arms[0] = {j.cx - adx, ay - 1.0f, -0.4f - w};
      arms[1] = {j.cx + adx, ay - 1.0f, 0.4f - w};
      break;
    }
    default: {
      float sway = sinf(t * 1.8f + 0.6f) * 0.08f;
      if (happy > 0) {
        float wig = sinf(t * 14.0f) * 0.25f;
        arms[0] = {j.cx - adx - 1.0f, j.cy - 4.0f, 2.4f + wig};
        arms[1] = {j.cx + adx + 1.0f, j.cy - 4.0f, -2.4f - wig};
      } else {
        arms[0] = {j.cx - adx, ay, -0.3f + sway};
        arms[1] = {j.cx + adx, ay, 0.3f - sway};
      }
      break;
    }
  }
  draw_avatar(j, arms, feet);

  float eye_y = j.fy - 0.5f;
  float eye_dx = j.fa * 0.48f;
  eye_style_t style = EYES_NORMAL;
  float open = 1.0f - blink;
  mouth_t mouth = MOUTH_SMILE;
  float mouth_open = 0;

  switch (mode) {
    case Mode::Listening:
      style = EYES_WIDE;
      mouth = MOUTH_O;
      break;
    case Mode::Thinking:
      open *= 0.85f;
      mouth = MOUTH_HMM;
      break;
    case Mode::Speaking:
      mouth = MOUTH_TALK;
      mouth_open = level * 1.3f + 0.1f * (0.5f + 0.5f * sinf(t * 22.0f));
      break;
    case Mode::Error:
      style = EYES_X;
      mouth = MOUTH_FLAT;
      break;
    case Mode::Off:
      open = clampf((1.0f - p.mode_t / 1.0f) * 1.5f, 0, 1);
      mouth = MOUTH_SMILE;
      break;
    default: break;
  }
  if (happy > 0.2f && mode != Mode::Error) {
    style = EYES_HAPPY;
    mouth = MOUTH_GRIN;
  }

  draw_eye(j.fx - eye_dx, eye_y, open, style, s_eyes.gx, s_eyes.gy);
  draw_eye(j.fx + eye_dx, eye_y, open, style, s_eyes.gx, s_eyes.gy);

  int bl = iround(j.fx - eye_dx), br = iround(j.fx + eye_dx), by = iround(eye_y) - 4;
  if (mode == Mode::Thinking) {
    px(bl - 1, by + 1, C_BROW);
    px(bl, by + 1, C_BROW);
    px(br - 1, by, C_BROW);
    px(br, by - 1, C_BROW);
  } else if (mode == Mode::Listening) {
    px(bl - 1, by - 1, C_BROW);
    px(bl, by - 1, C_BROW);
    px(br - 1, by - 1, C_BROW);
    px(br, by - 1, C_BROW);
  }

  float blush = 0.55f + happy * 0.45f + (mode == Mode::Speaking ? 0.15f : 0.0f);
  draw_blush(iround(j.fx - j.fa * 0.72f), iround(eye_y + 2), blush);
  draw_blush(iround(j.fx + j.fa * 0.72f), iround(eye_y + 2), blush);

  draw_mouth(iround(j.fx), iround(eye_y + 3), mouth, mouth_open);

  float top = j.cy - j.b;
  if (mode == Mode::Thinking) draw_thought_dots(j.cx + 14, top + 2, t);
  if (mode == Mode::Error) draw_alert(iround(j.cx + 18), iround(top - 1));
}

Mode modeFor(Expr e) {
  switch (e) {
    case Expr::Think: return Mode::Thinking;
    case Expr::Alert:
    case Expr::Worried: return Mode::Error;
    case Expr::Sleepy: return Mode::Off;
    default: return Mode::Idle;
  }
}

float happyFor(Expr e) { return e == Expr::Celebrate ? 1.0f : 0.0f; }

class Jolly : public Character {
 public:
  const char* id() const override { return "jolly"; }
  const char* name() const override { return "jolly"; }
  const char* category() const override { return "muse"; }

  void draw(Gfx& g, int cx, int cy, int size, const MascotState& st, uint32_t tickMs) override {
    const Mode mode = modeFor(st.expr);
    if (mode != mode_) {
      mode_ = mode;
      since_ = tickMs;
    }
    render({mode, tickMs * 0.001f, (tickMs - since_) * 0.001f, 0.0f, happyFor(st.expr)});

    const int box = size * 32 / 22;
    const int ox = cx - box / 2;
    const int oy = cy - box / 2 - st.liftPx;
    if (st.shadow) {
      drawShadow(g, cx, cy - box / 2 + box * 59 / 64, box, std::max(1, size / 22), st.liftPx / 12);
    }
    auto& c = g.c();
    for (int y = 0; y < H; y++) {
      const int y0 = oy + y * box / H;
      const int h = oy + (y + 1) * box / H - y0;
      const uint8_t* row = &s_fb[y * W];
      for (int x = 0; x < W;) {
        const uint8_t idx = row[x];
        int run = x + 1;
        while (run < W && row[run] == idx) ++run;
        if (idx != C_BG) {
          const int x0 = ox + x * box / W;
          c.fillRect(x0, y0, ox + run * box / W - x0, h, s_pal[idx]);
        }
        x = run;
      }
    }
  }

 private:
  Mode mode_ = Mode::Idle;
  uint32_t since_ = 0;
};

}  // namespace

namespace characters {

Character& jolly() {
  static Jolly instance;
  return instance;
}

}  // namespace characters

}  // namespace tama
