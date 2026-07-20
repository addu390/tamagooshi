#include "brand.gen.h"
#if TAMA_APP_POMODORO

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kStepMin = 5;
constexpr int kWorkMinMin = 5, kWorkMaxMin = 30;
constexpr int kBreakMinMin = 5, kBreakMaxMin = 15;
constexpr float kSmoothing = 0.25f;
constexpr uint32_t kDwellMs = 400;
constexpr uint32_t kFlipDwellMs = 700;
constexpr uint32_t kTiltRepeatMs = 450;

// The screen is the hourglass: a 15x10 grid of sand boxes, so one box always
// stands for exactly 1/150th of the session. Boxes detach in reading order,
// fall down their own column, and stack in the same order at the bottom.
constexpr int kCell = 9;
constexpr int kCols = 15;
constexpr int kRows = 10;
constexpr int kCells = kCols * kRows;
constexpr float kFallPxPerSec = 180.0f;

enum class Session : uint8_t { Work, Break };
enum class Posture : uint8_t { None, Upright, Flipped, Flat, TiltLeft, TiltRight };

const char* label(Session s) { return s == Session::Work ? "WORK" : "BREAK"; }

int perCol(int grains, int col) {
  return grains <= col ? 0 : (grains - col - 1) / kCols + 1;
}

class PomodoroScreen : public AppScreen {
 public:
  const char* id() const override { return "app.pomodoro"; }
  OrientationPref orientation() const override {
    return flipped_ && !setup_ ? OrientationPref::PortraitFlipped : OrientationPref::Portrait;
  }

  void onEnter(ShellContext& ctx) override {
    sensor_ = &ctx.sensor;
    hasImu_ = ctx.caps.imu;
    workMin_ = 25;
    breakMin_ = 5;
    toSetup();
    posture_ = Posture::None;
    candidate_ = Posture::None;
    haveSample_ = false;
    lastTick_ = 0;
  }

  void onExit() override { sensor_ = nullptr; }

  void render(Gfx& g, ShellContext& ctx) override {
    if (setup_) {
      renderSetup(g, ctx);
    } else {
      renderSand(g);
    }
  }

  Transition handleInput(Intent intent, ShellContext&) override {
    if (intent == Intent::Select) {
      if (!setup_) {
        running_ = !running_;
      } else if (sel_ == 2) {
        begin();
      } else {
        bump(sel_ == 0 ? Session::Work : Session::Break, kStepMin);
      }
      return Transition::redraw();
    }
    if (intent == Intent::Next) {
      if (setup_) {
        sel_ = (sel_ + 1) % 3;
      } else {
        toSetup();
      }
      return Transition::redraw();
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    if (lastTick_ == 0) lastTick_ = nowMs;
    const uint32_t dt = nowMs - lastTick_;
    lastTick_ = nowMs;

    if (!setup_ && running_) {
      if (remainingMs_ <= dt) {
        finishSession(ctx);
      } else {
        remainingMs_ -= dt;
      }
    }

    if (hasImu_) trackPosture(nowMs);

    if (!anim_.due(nowMs, !setup_ && running_ ? 33 : 150)) return Transition::none();
    return Transition::redraw();
  }

 private:
  uint32_t durationMs() const {
    return static_cast<uint32_t>(session_ == Session::Work ? workMin_ : breakMin_) * 60000u;
  }

  uint32_t quantumMs() const { return durationMs() / kCells; }

  void begin() {
    setup_ = false;
    session_ = Session::Work;
    remainingMs_ = durationMs();
    running_ = true;
    flipped_ = false;
  }

  void toSetup() {
    setup_ = true;
    running_ = false;
    flipped_ = false;
    sel_ = 2;
    session_ = Session::Work;
    remainingMs_ = durationMs();
  }

  void switchSession(ShellContext* ctx) {
    if (ctx != nullptr && ctx->expression && !ctx->state.muted) {
      const ExpressionKind kind =
          session_ == Session::Work ? ExpressionKind::Celebrate : ExpressionKind::Chirp;
      ctx->expression->play({kind, 100, 0});
    }
    session_ = session_ == Session::Work ? Session::Break : Session::Work;
    remainingMs_ = durationMs();
  }

  void finishSession(ShellContext& ctx) { switchSession(&ctx); }

  void bump(Session which, int delta) {
    int& value = which == Session::Work ? workMin_ : breakMin_;
    const int lo = which == Session::Work ? kWorkMinMin : kBreakMinMin;
    const int hi = which == Session::Work ? kWorkMaxMin : kBreakMaxMin;
    value += delta;
    if (value > hi) value = lo;
    if (value < lo) value = hi;
    if (setup_) remainingMs_ = durationMs();
  }

  void trackPosture(uint32_t nowMs) {
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    if (!sensor_ || !sensor_->accel(ax, ay, az)) return;
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (mag < 0.5f) return;
    ax /= mag;
    ay /= mag;
    az /= mag;
    if (!haveSample_) {
      fx_ = ax;
      fy_ = ay;
      fz_ = az;
      haveSample_ = true;
    } else {
      fx_ += (ax - fx_) * kSmoothing;
      fy_ += (ay - fy_) * kSmoothing;
      fz_ += (az - fz_) * kSmoothing;
    }

    Posture p = Posture::None;
    if (std::fabs(fz_) > 0.75f) {
      p = Posture::Flat;
    } else if (std::fabs(fy_) > 0.6f) {
      p = fy_ > 0 ? Posture::Upright : Posture::Flipped;
    } else if (std::fabs(fx_) > 0.6f) {
      p = fx_ > 0 ? Posture::TiltRight : Posture::TiltLeft;
    }

    if (p != candidate_) {
      candidate_ = p;
      candidateSince_ = nowMs;
      return;
    }
    const uint32_t dwell = p == Posture::Flipped ? kFlipDwellMs : kDwellMs;
    if (nowMs - candidateSince_ < dwell) return;

    const bool tilt = p == Posture::TiltLeft || p == Posture::TiltRight;
    if (tilt && setup_) {
      if (sel_ != 2 && nowMs - lastTiltStep_ >= kTiltRepeatMs) {
        lastTiltStep_ = nowMs;
        bump(sel_ == 0 ? Session::Work : Session::Break,
             p == Posture::TiltRight ? kStepMin : -kStepMin);
      }
      return;
    }
    if (p == posture_) return;
    posture_ = p;

    if (setup_) {
      if (p == Posture::Upright) begin();
      return;
    }
    switch (p) {
      case Posture::Upright:
        if (flipped_) switchSession(nullptr);
        flipped_ = false;
        running_ = true;
        break;
      case Posture::Flipped:
        if (!flipped_) switchSession(nullptr);
        flipped_ = true;
        running_ = true;
        break;
      case Posture::Flat:
        running_ = false;
        break;
      default:
        break;
    }
  }

  uint16_t accent() const { return session_ == Session::Work ? theme::kHi : theme::kBlush; }

  void renderSand(Gfx& g) {
    g.clear();
    auto& c = g.c();
    const int w = g.w();
    const int h = g.h();
    const uint16_t col = accent();

    const uint32_t q = quantumMs();
    const uint32_t elapsed = durationMs() - remainingMs_;
    const int k = std::min(static_cast<int>(elapsed / q), kCells - 1);
    const int gone = k + 1;
    const int landed = k;

    int minMassH = kRows * kCell;
    for (int i = 0; i < kCols; ++i) {
      const int x = i * kCell;
      const int massH = (kRows - perCol(gone, i)) * kCell;
      if (massH > 0) c.fillRect(x, 0, kCell, massH, col);
      if (massH < minMassH) minMassH = massH;
      const int pileH = perCol(landed, i) * kCell;
      if (pileH > 0) c.fillRect(x, h - pileH, kCell, pileH, col);
    }

    const int gcol = k % kCols;
    const int startY = (kRows - perCol(gone, gcol)) * kCell;
    const int restY = h - perCol(landed, gcol) * kCell - kCell;
    const uint32_t tIn = elapsed % q;
    int gy = startY + static_cast<int>(kFallPxPerSec * tIn / 1000.0f);
    if (gy > restY) gy = restY;
    c.fillRect(gcol * kCell + 1, gy + 1, kCell - 2, kCell - 2, col);

    char mmss[6];
    const uint32_t s = remainingMs_ / 1000;
    std::snprintf(mmss, sizeof(mmss), "%02u:%02u", s / 60, s % 60);
    const uint16_t txt = minMassH >= 20 ? theme::kInk : theme::kFg;
    g.str(label(session_), 8, 6, txt, typeface::micro());
    g.str(mmss, w - 8, 6, txt, typeface::micro(), textdatum_t::top_right);

    if (!running_) {
      g.str("PAUSED", w / 2, h / 2, theme::kFg, typeface::micro(), textdatum_t::middle_center);
    }
  }

  void renderSetup(Gfx& g, ShellContext& ctx) {
    const auto L = widgets::frame(g, ctx.state, "POMODORO");

    char work[8], brk[8];
    std::snprintf(work, sizeof(work), "%d MIN", workMin_);
    std::snprintf(brk, sizeof(brk), "%d MIN", breakMin_);
    const widgets::ListItem items[] = {
        {"WORK", work},
        {"BREAK", brk},
        {"START", ""},
    };
    widgets::listView(g, L, items, 3, sel_);

    widgets::hints(g, sel_ == 2 ? "START" : "+5 MIN", "NEXT");
  }

  AnimClock anim_;
  ISensorSource* sensor_ = nullptr;
  bool hasImu_ = false;
  bool setup_ = true;
  Session session_ = Session::Work;
  int workMin_ = 25;
  int breakMin_ = 5;
  uint32_t remainingMs_ = 0;
  bool running_ = false;
  bool flipped_ = false;
  int sel_ = 0;
  uint32_t lastTick_ = 0;
  Posture posture_ = Posture::None;
  Posture candidate_ = Posture::None;
  uint32_t candidateSince_ = 0;
  uint32_t lastTiltStep_ = 0;
  bool haveSample_ = false;
  float fx_ = 0.0f;
  float fy_ = 0.0f;
  float fz_ = 1.0f;
};

}  // namespace

AppScreen& pomodoro() {
  static PomodoroScreen instance;
  return instance;
}

}  // namespace tama::apps

#endif  // TAMA_APP_POMODORO
