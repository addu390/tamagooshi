#include "brand.gen.h"
#if TAMA_APP_TALK

#include <cstdio>
#include <string>
#include <vector>

#include "apps.h"
#include "mascot.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {

namespace {

constexpr int kChunkSamples = 256;
constexpr int kChunksPerTick = 6;
constexpr int kLineH = 12;

class TalkScreen : public AppScreen {
 public:
  const char* id() const override { return "app.talk"; }

  void onEnter(ShellContext& ctx) override {
    mic_ = &ctx.mic;
    scroll_ = 0;
    const VoicePhase phase = ctx.state.voice.phase;
    if (phase != VoicePhase::Thinking && phase != VoicePhase::Reply) ctx.state.voice.reset();
  }

  void onExit() override {
    if (recording_) stopMic();
    mic_ = nullptr;
  }

  void render(Gfx& g, ShellContext& ctx) override {
    const auto L = widgets::frame(g, ctx.state, title(ctx).c_str());
    switch (ctx.state.voice.phase) {
      case VoicePhase::Recording: renderRecording(g, ctx, L); break;
      case VoicePhase::Sending: renderSending(g, ctx, L); break;
      case VoicePhase::Thinking: renderThinking(g, ctx, L); break;
      case VoicePhase::Reply: renderReply(g, ctx, L); break;
      case VoicePhase::Idle: renderIdle(g, ctx, L); break;
    }
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    switch (ctx.state.voice.phase) {
      case VoicePhase::Idle:
        if (intent == Intent::Select && canRecord(ctx)) {
          startRecording(ctx);
          return Transition::redraw();
        }
        if (intent == Intent::Next) return Transition::back();
        break;
      case VoicePhase::Recording:
        if (intent == Intent::Select) {
          finishRecording(ctx);
          return Transition::redraw();
        }
        if (intent == Intent::Next) {
          cancelRecording(ctx);
          return Transition::redraw();
        }
        break;
      case VoicePhase::Reply:
        if (intent == Intent::Select && canRecord(ctx)) {
          ctx.state.voice.reset();
          startRecording(ctx);
          return Transition::redraw();
        }
        if (intent == Intent::Next) {
          if (overflow_ > 0) {
            scroll_ = scroll_ >= overflow_ ? 0 : scroll_ + 1;
            return Transition::redraw();
          }
          return Transition::back();
        }
        break;
      default: break;
    }
    return Transition::none();
  }

  Transition tick(ShellContext& ctx, uint32_t nowMs) override {
    now_ = nowMs;
    if (recording_) {
      drain(ctx);
      return anim_.due(nowMs, 90) ? Transition::redraw() : Transition::none();
    }
    const VoiceChat& voice = ctx.state.voice;
    if (voice.phase == VoicePhase::Reply && voice.reply_done) {
      if (!settled_) {
        settled_ = true;
        cue(ctx, voice.error.empty() ? ExpressionKind::Celebrate : ExpressionKind::Warn);
      }
      return Transition::none();
    }
    settled_ = false;
    const bool animating = voice.phase != VoicePhase::Idle;
    return animating && anim_.due(nowMs, 250) ? Transition::redraw() : Transition::none();
  }

 private:
  static std::string title(ShellContext& ctx) {
    if (!ctx.assistant) return "TALK";
    const std::string name = ctx.assistant->name();
    return name.empty() ? "TALK" : "TALK: " + widgets::upper(name.c_str());
  }

  static bool canRecord(ShellContext& ctx) {
    return ctx.caps.mic && ctx.voice != nullptr && ctx.voice->ready();
  }

  void startRecording(ShellContext& ctx) {
    ctx.voice->beginRecording();
    mic_->begin();
    if (!mic_->startRecord()) {
      mic_->end();
      ctx.voice->cancel();
      cue(ctx, ExpressionKind::Warn);
      return;
    }
    recording_ = true;
    samples_ = 0;
    level_ = 0;
    settled_ = false;
    ctx.state.voice.phase = VoicePhase::Recording;
    cue(ctx, ExpressionKind::Chirp);
  }

  void finishRecording(ShellContext& ctx) {
    stopMic();
    ctx.voice->finish(elapsedMs(ctx));
    if (ctx.voice->sending()) {
      ctx.state.voice.phase = VoicePhase::Sending;
      cue(ctx, ExpressionKind::Tick);
    } else {
      ctx.state.voice.reset();
      cue(ctx, ExpressionKind::Warn);
    }
  }

  void cancelRecording(ShellContext& ctx) {
    stopMic();
    ctx.voice->cancel();
    ctx.state.voice.reset();
    cue(ctx, ExpressionKind::Warn);
  }

  void stopMic() {
    recording_ = false;
    if (mic_) {
      mic_->stopRecord();
      mic_->end();
    }
  }

  void drain(ShellContext& ctx) {
    int16_t pcm[kChunkSamples];
    for (int i = 0; i < kChunksPerTick; ++i) {
      const size_t n = mic_->readRecord(pcm, kChunkSamples);
      if (n == 0) break;
      ctx.voice->feed(pcm, n);
      samples_ += n;
      int32_t sum = 0;
      for (size_t j = 0; j < n; ++j) sum += pcm[j] < 0 ? -pcm[j] : pcm[j];
      level_ = static_cast<int>(sum / static_cast<int32_t>(n));
    }
    if (elapsedMs(ctx) >= ctx.voice->limitMs()) finishRecording(ctx);
  }

  uint32_t elapsedMs(ShellContext& ctx) const {
    return static_cast<uint32_t>(samples_ * 1000ull / ctx.mic.recordRate());
  }

  static std::string clock(uint32_t ms) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%u:%02u", ms / 60000u, (ms / 1000u) % 60u);
    return buf;
  }

  std::string dots() const {
    const int n = 1 + static_cast<int>((now_ / 350u) % 3u);
    return std::string(static_cast<size_t>(n), '.');
  }

  struct Stage {
    int mx;
    int my;
    int size;
    int tx;
    int ty;
    int tw;
  };

  static Stage stage(const widgets::Layout& L) {
    if (L.landscape) {
      return {widgets::anchor(L, widgets::Side::Left), L.cy + 4, 44, L.w * 5 / 8, L.cy - 16,
              L.w / 2 - 12};
    }
    return {L.cx, L.top + 60, 66, L.cx, L.top + 116, L.w - 16};
  }

  void drawMascot(Gfx& g, ShellContext& ctx, const Stage& st, Expr expr) const {
    if (!ctx.character) return;
    MascotState m{expr};
    m.wanderPx = 4;
    ctx.character->draw(g, st.mx, st.my, st.size, m, now_);
  }

  void renderIdle(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const bool ok = canRecord(ctx);
    const Stage st = stage(L);
    drawMascot(g, ctx, st, ok ? Expr::Happy : Expr::Sleepy);
    g.str(ok ? "ready to talk" : "agent offline", st.tx, st.ty, ok ? theme::kFg : theme::kDim,
          typeface::body(), textdatum_t::top_center);
    g.str(ok ? "press A and speak" : "pair it in settings", st.tx, st.ty + 18,
          ok ? theme::kDim : theme::kDimmer, typeface::micro(), textdatum_t::top_center);
    widgets::hints(g, ok ? "REC" : nullptr, "BACK");
  }

  void renderRecording(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const Stage st = stage(L);
    drawMascot(g, ctx, st, Expr::Neutral);

    const uint32_t ms = elapsedMs(ctx);
    const std::string elapsed = clock(ms);
    g.str(elapsed.c_str(), st.tx + 6, st.ty, theme::kHi, typeface::title(),
          textdatum_t::top_center);
    if ((now_ / 400u) % 2u == 0) {
      const int dotX = st.tx + 6 - g.textWidth(elapsed.c_str(), typeface::title()) / 2 - 10;
      g.c().fillCircle(dotX, st.ty + 9, 4, theme::kCrit);
    }

    const int meterW = st.tw - 8;
    const int meterX = st.tx - meterW / 2;
    const int meterY = st.ty + 28;
    int len = meterW * level_ / 900;
    if (len > meterW) len = meterW;
    g.c().drawRect(meterX, meterY, meterW, 8, theme::kDim);
    if (len > 2) g.c().fillRect(meterX + 1, meterY + 1, len - 2, 6, theme::kHi);

    const uint32_t limit = ctx.voice->limitMs();
    const std::string left = clock(limit > ms ? limit - ms : 0) + " left";
    g.str(left.c_str(), st.tx, meterY + 14, theme::kDim, typeface::micro(),
          textdatum_t::top_center);

    widgets::hints(g, "SEND", "BACK");
  }

  void renderSending(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    renderWaiting(g, ctx, L, "sending");
  }

  void renderThinking(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    renderWaiting(g, ctx, L, "thinking");
  }

  void renderWaiting(Gfx& g, ShellContext& ctx, const widgets::Layout& L, const char* verb) {
    const Stage st = stage(L);
    drawMascot(g, ctx, st, Expr::Think);
    const std::string msg = verb + dots();
    const int textW = g.textWidth(verb, typeface::body());
    g.str(msg.c_str(), st.tx - textW / 2, st.ty, theme::kFg, typeface::body(),
          textdatum_t::top_left);
    widgets::wrapText(g, ctx.state.voice.transcript.c_str(), st.tx, st.ty + 20, st.tw,
                      typeface::micro(), theme::kDim, 11, L.bottom - 2);
    widgets::hints(g, nullptr, nullptr);
  }

  void renderReply(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const VoiceChat& voice = ctx.state.voice;
    const bool failed = !voice.error.empty();
    const std::vector<std::string> lines =
        widgets::wrap(g, failed ? voice.error : voice.reply, L.w - 12, typeface::micro());

    const int top = L.top + 22;
    const int visible = (L.bottom - top - 4) / kLineH;
    const int total = static_cast<int>(lines.size());
    overflow_ = total > visible ? total - visible : 0;
    if (scroll_ > overflow_) scroll_ = overflow_;

    int y = top;
    for (int i = scroll_; i < total && i < scroll_ + visible; ++i) {
      g.str(lines[i].c_str(), 6, y, failed ? theme::kCrit : theme::kFg, typeface::micro(),
            textdatum_t::top_left);
      y += kLineH;
    }
    if (!voice.reply_done) {
      g.str(dots().c_str(), L.cx, L.bottom - 10, theme::kDim, typeface::body(),
            textdatum_t::top_center);
    }
    widgets::hints(g, "TALK", overflow_ > 0 ? "MORE" : "BACK");
  }

  IMicSource* mic_ = nullptr;
  bool recording_ = false;
  bool settled_ = false;
  uint64_t samples_ = 0;
  int level_ = 0;
  int scroll_ = 0;
  int overflow_ = 0;
  uint32_t now_ = 0;
  AnimClock anim_;
};

}  // namespace

TAMA_SCREEN_FACTORY(talk, TalkScreen)

}  // namespace tama::apps

#endif  // TAMA_APP_TALK
