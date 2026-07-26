#include "brand.gen.h"
#if TAMA_GAME_SIMON

#include <cstdio>
#include <vector>

#include "arcade.h"
#include "games.h"

namespace tama::games {

namespace {

enum class Phase : uint8_t { Show, Gap, Listen, Feedback, Advance };

class SimonScreen : public ArcadeGameScreen {
 public:
  SimonScreen() : ArcadeGameScreen(OrientationPref::Portrait) { rng_.seed(0xc0ffee42u); }
  const char* id() const override { return "game.simon"; }

 protected:
  const char* title() const override { return "SIMON"; }
  const char* readyHint() const override { return "WATCH THEN COPY"; }
  const char* runHint() const override {
    if (phase_ == Phase::Listen) return "A";
    if (phase_ == Phase::Advance) return "NEXT";
    return "WAIT";
  }
  const char* hintB() const override {
    return st_ == St::Run && phase_ == Phase::Listen ? "B" : nullptr;
  }
  const char* deadTitle() const override { return "WRONG"; }
  int bannerCenterY() const override { return 58; }
  const lgfx::IFont* bannerHeadFont() const override { return typeface::body(); }

  void renderWorld(Gfx& g, ShellContext&) override {
    auto& c = g.c();
    const int padW = (w_ - 36) / 2;
    const int padH = 72;
    const int y = h_ / 2 - padH / 2 + 8;
    const int leftX = 12;
    const int rightX = w_ - 12 - padW;

    if (st_ == St::Run && phase_ == Phase::Advance) {
      const int round =
          growPending_ ? static_cast<int>(seq_.size()) + 1 : static_cast<int>(seq_.size());
      char label[12];
      std::snprintf(label, sizeof(label), "ROUND %d", round);
      g.str(label, w_ / 2, y - 28, theme::kHi, typeface::body(), textdatum_t::middle_center);
      g.str("WATCH", w_ / 2, y - 12, theme::kDim, typeface::micro(), textdatum_t::middle_center);
    }

    drawPad(c, leftX, y, padW, padH, 0, lit(0));
    drawPad(c, rightX, y, padW, padH, 1, lit(1));

    g.str("A", leftX + padW / 2, y + padH + 14, theme::kDim, typeface::micro(),
          textdatum_t::top_center);
    g.str("B", rightX + padW / 2, y + padH + 14, theme::kDim, typeface::micro(),
          textdatum_t::top_center);
  }

  Transition onAction(Intent intent, ShellContext& ctx) override {
    if (phase_ != Phase::Listen) return Transition::none();

    int pad = -1;
    if (intent == Intent::Select) pad = 0;
    else if (intent == Intent::Next) pad = 1;
    else return Transition::none();

    if (pad != seq_[inputAt_]) {
      cue(ctx, ExpressionKind::Warn);
      die();
      return Transition::redraw();
    }

    flash_ = pad;
    phase_ = Phase::Feedback;
    phaseMs_ = 0;
    ++inputAt_;
    cue(ctx, ExpressionKind::Tick);

    if (inputAt_ >= static_cast<int>(seq_.size())) {
      score_ = static_cast<int>(seq_.size());
      growPending_ = true;
    }
    return Transition::redraw();
  }

  void onReset() override {
    seq_.clear();
    seq_.push_back(static_cast<uint8_t>(rng_.next() & 1u));
    inputAt_ = 0;
    showAt_ = 0;
    flash_ = -1;
    phase_ = Phase::Advance;
    phaseMs_ = 0;
    growPending_ = false;
  }

  void step(ShellContext& ctx) override {
    phaseMs_ += kStepMs;

    switch (phase_) {
      case Phase::Advance:
        flash_ = -1;
        if (phaseMs_ >= kAdvanceMs) {
          if (growPending_) {
            growPending_ = false;
            seq_.push_back(static_cast<uint8_t>(rng_.next() & 1u));
          }
          showAt_ = 0;
          inputAt_ = 0;
          phase_ = Phase::Gap;
          phaseMs_ = 0;
          cue(ctx, ExpressionKind::Blink);
        }
        break;

      case Phase::Gap:
        flash_ = -1;
        if (phaseMs_ >= kGapMs) {
          if (showAt_ >= static_cast<int>(seq_.size())) {
            phase_ = Phase::Listen;
            inputAt_ = 0;
            phaseMs_ = 0;
          } else {
            phase_ = Phase::Show;
            flash_ = seq_[showAt_];
            phaseMs_ = 0;
            cue(ctx, ExpressionKind::Tick);
          }
        }
        break;

      case Phase::Show:
        if (phaseMs_ >= kShowMs) {
          ++showAt_;
          phase_ = Phase::Gap;
          phaseMs_ = 0;
          flash_ = -1;
        }
        break;

      case Phase::Feedback:
        if (phaseMs_ >= kFeedbackMs) {
          flash_ = -1;
          phaseMs_ = 0;
          if (growPending_) {
            phase_ = Phase::Advance;
          } else {
            phase_ = Phase::Listen;
          }
        }
        break;

      case Phase::Listen:
        break;
    }
  }

 private:
  void drawPad(M5Canvas& c, int x, int y, int w, int h, int pad, bool on) const {
    const uint16_t fill = on ? theme::kHi : theme::kDimmer;
    const uint16_t edge = on ? theme::kFg : theme::kDim;
    c.fillRoundRect(x, y, w, h, 8, fill);
    c.drawRoundRect(x, y, w, h, 8, edge);
    if (on) {
      c.drawRoundRect(x + 2, y + 2, w - 4, h - 4, 6, theme::kBg);
    }
    (void)pad;
  }

  bool lit(int pad) const {
    if (st_ != St::Run) return false;
    if (phase_ == Phase::Advance) return (phaseMs_ / 120) % 2 == 0;
    return flash_ == pad;
  }

  static constexpr uint32_t kShowMs = 420;
  static constexpr uint32_t kGapMs = 220;
  static constexpr uint32_t kFeedbackMs = 180;
  static constexpr uint32_t kAdvanceMs = 900;

  std::vector<uint8_t> seq_;
  int inputAt_ = 0;
  int showAt_ = 0;
  int flash_ = -1;
  Phase phase_ = Phase::Advance;
  uint32_t phaseMs_ = 0;
  bool growPending_ = false;
};

}  // namespace

TAMA_SCREEN_FACTORY(simon, SimonScreen)

}  // namespace tama::games

#endif  // TAMA_GAME_SIMON
