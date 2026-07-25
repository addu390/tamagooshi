#include "brand.gen.h"
#if TAMA_APP_SOUND

#include <algorithm>
#include <cstdio>

#include "apps.h"
#include "audio.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kMinDelta = 200;
constexpr uint32_t kSampleMs = 50;
constexpr int kHistory = 48;

class SoundScreen : public AppScreen {
 public:
  const char* id() const override { return "app.sound"; }

  void onEnter(ShellContext& ctx) override {
    mic_ = &ctx.mic;
    mic_->begin();
    meter_.reset();
    peak_ = 0;
    histLen_ = 0;
    histAt_ = 0;
  }

  void onExit() override {
    if (mic_) {
      mic_->end();
      mic_ = nullptr;
    }
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    AppScreen::tick(ctx, nowMs);
    if (!sampler_.due(nowMs, kSampleMs)) return Transition::none();

    meter_.sample(mic_);
    if (meter_.level() > peak_) {
      peak_ = meter_.level();
    } else {
      peak_ -= (peak_ - meter_.level()) / 32;
    }

    hist_[histAt_] = pct(meter_.level());
    histAt_ = (histAt_ + 1) % kHistory;
    if (histLen_ < kHistory) ++histLen_;
    return Transition::redraw();
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, "SOUND");
    auto& c = g.c();

    const bool loud = meter_.loud();
    widgets::pill(g, L.cx, L.top + 20, loud ? "LOUD" : "QUIET", typeface::micro(),
                  loud ? theme::kWarn : theme::kHi);

    const int barY = L.top + 44;
    const int barW = L.w - 32;
    const int fill = barW * pct(meter_.level()) / 100;
    const int peakX = 16 + barW * pct(peak_) / 100;
    c.drawRect(16, barY, barW, 12, theme::kDim);
    if (fill > 0) c.fillRect(16, barY, fill, 12, loud ? theme::kWarn : theme::kHi);
    c.drawFastVLine(peakX, barY - 2, 16, theme::kFg);

    const int graphTop = barY + 24;
    const int graphH = L.bottom - graphTop - 16;
    const int colW = std::max(1, barW / kHistory);
    for (int i = 0; i < histLen_; ++i) {
      const int v = hist_[(histAt_ + kHistory - histLen_ + i) % kHistory];
      const int hgt = std::max(1, graphH * v / 100);
      c.fillRect(16 + i * colW, graphTop + graphH - hgt, colW - 1 > 0 ? colW - 1 : 1, hgt,
                 theme::kDim);
    }

    char lvl[16];
    std::snprintf(lvl, sizeof(lvl), "LVL %d", meter_.level());
    g.str(lvl, L.cx, L.bottom - 6, theme::kDimmer, typeface::micro(), textdatum_t::middle_center);

    widgets::hints(g, nullptr, nullptr);
  }

 private:
  int pct(int level) const {
    const int span = meter_.threshold() * 2;
    const int p = span > 0 ? 100 * level / span : 0;
    return std::clamp(p, 0, 100);
  }

  AnimClock sampler_;
  IMicSource* mic_ = nullptr;
  audio::LevelMeter meter_{kMinDelta};
  int peak_ = 0;
  int hist_[kHistory] = {};
  int histLen_ = 0;
  int histAt_ = 0;
};

}  // namespace

TAMA_SCREEN_FACTORY(sound, SoundScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_SOUND
