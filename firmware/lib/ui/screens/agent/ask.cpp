#include "brand.gen.h"
#if defined(TAMA_ENABLE_BUDDY)

#include <cctype>
#include <cstdio>
#include <string>
#include <vector>

#include "mascot.h"
#include "screens.h"
#include "theme.h"
#include "widgets.h"

namespace tama::screens {

namespace {

constexpr int kChunkSamples = 256;
constexpr int kChunksPerTick = 6;
constexpr int kLineH = 12;

void wrapLines(Gfx& g, const std::string& text, int maxWidth, const lgfx::IFont* font,
               std::vector<std::string>& out) {
  out.clear();
  std::string line;
  size_t start = 0;
  while (start <= text.size()) {
    const size_t sp = text.find(' ', start);
    const std::string word =
        text.substr(start, sp == std::string::npos ? std::string::npos : sp - start);
    const std::string cand = line.empty() ? word : line + " " + word;
    if (line.empty() || g.textWidth(cand.c_str(), font) <= maxWidth) {
      line = cand;
    } else {
      out.push_back(line);
      line = word;
    }
    if (sp == std::string::npos) break;
    start = sp + 1;
  }
  if (!line.empty()) out.push_back(line);
}

class AskScreen : public AppScreen {
 public:
  const char* id() const override { return "ask"; }

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
    const auto L = widgets::frame(g, ctx.state, headerTitle(ctx).c_str());
    switch (ctx.state.voice.phase) {
      case VoicePhase::Recording: renderRecording(g, ctx, L); break;
      case VoicePhase::Sending: renderBusy(g, ctx, L, "sending"); break;
      case VoicePhase::Confirming:
      case VoicePhase::Thinking: renderThinking(g, ctx, L); break;
      case VoicePhase::Reply: renderReply(g, ctx, L); break;
      case VoicePhase::Idle:
      default: renderIdle(g, ctx, L); break;
    }
  }

  Transition handleInput(Intent intent, ShellContext& ctx) override {
    switch (ctx.state.voice.phase) {
      case VoicePhase::Idle:
        if (intent == Intent::Select && canRecord(ctx)) {
          startRecording(ctx);
          return Transition::redraw();
        }
        if (intent == Intent::Next) {
          if (ctx.state.agents.multiple()) {
            ctx.state.agents.cycle();
            return Transition::redraw();
          }
          return Transition::back();
        }
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
    const VoicePhase phase = ctx.state.voice.phase;
    const bool animating = phase == VoicePhase::Sending || phase == VoicePhase::Thinking ||
                           (phase == VoicePhase::Reply && !ctx.state.voice.reply_done);
    if (animating) {
      return anim_.due(nowMs, 250) ? Transition::redraw() : Transition::none();
    }
    return Transition::none();
  }

 private:
  static std::string upper(const std::string& s) {
    std::string out;
    for (char c : s) out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
  }

  static const std::string& activeAgent(ShellContext& ctx) {
    const std::string& answering = ctx.state.voice.agent;
    return answering.empty() ? ctx.state.agents.current() : answering;
  }

  static std::string headerTitle(ShellContext& ctx) {
    std::string title = "ASK";
    const std::string& agent = activeAgent(ctx);
    if (!agent.empty()) title += ": " + upper(agent);
    return title;
  }

  bool canRecord(ShellContext& ctx) const {
    return ctx.caps.mic && ctx.voice != nullptr && ctx.voice->ready();
  }

  void startRecording(ShellContext& ctx) {
    ctx.voice->beginRecording();
    mic_->begin();
    if (!mic_->startRecord()) {
      mic_->end();
      ctx.voice->cancel();
      return;
    }
    recording_ = true;
    samples_ = 0;
    level_ = 0;
    ctx.state.voice.phase = VoicePhase::Recording;
  }

  void finishRecording(ShellContext& ctx) {
    stopMic();
    ctx.voice->finish(elapsedMs(ctx), ctx.state.agents.current());
    if (ctx.voice->sending()) {
      ctx.state.voice.phase = VoicePhase::Sending;
    } else {
      ctx.state.voice.reset();
    }
  }

  void cancelRecording(ShellContext& ctx) {
    stopMic();
    ctx.voice->cancel();
    ctx.state.voice.reset();
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
    if (ctx.voice->bufferedBytes() >= ctx.voice->capacityBytes()) finishRecording(ctx);
  }

  uint32_t elapsedMs(ShellContext& ctx) const {
    return static_cast<uint32_t>(samples_ * 1000ull / ctx.mic.recordRate());
  }

  uint32_t maxMs(ShellContext& ctx) const {
    return static_cast<uint32_t>(ctx.voice->capacityBytes() * 2ull * 1000ull /
                                 ctx.mic.recordRate());
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

  void renderIdle(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    const bool ok = canRecord(ctx);
    const bool picker = ctx.state.agents.multiple();
    if (ctx.character) {
      ctx.character->draw(g, L.cx, L.cy - 18, 44, MascotState{ok ? Expr::Happy : Expr::Sleepy},
                          now_);
    }
    if (ok) {
      if (picker) {
        widgets::pill(g, L.cx, L.cy + 12, upper(ctx.state.agents.current()).c_str(),
                      typeface::micro(), theme::kHi);
        g.str("press A and speak", L.cx, L.cy + 32, theme::kDim, typeface::micro(),
              textdatum_t::top_center);
      } else {
        g.str("ask your agent", L.cx, L.cy + 16, theme::kFg, typeface::body(),
              textdatum_t::top_center);
        g.str("press A and speak", L.cx, L.cy + 32, theme::kDim, typeface::micro(),
              textdatum_t::top_center);
      }
    } else {
      g.str("agent offline", L.cx, L.cy + 16, theme::kDim, typeface::body(),
            textdatum_t::top_center);
      g.str("pair and start the hub", L.cx, L.cy + 32, theme::kDimmer, typeface::micro(),
            textdatum_t::top_center);
    }
    widgets::hints(g, ok ? "REC" : nullptr, picker ? "AGENT" : "BACK");
  }

  void renderRecording(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    if ((now_ / 400u) % 2u == 0) g.c().fillCircle(L.cx - 30, L.top + 26, 4, theme::kCrit);
    g.str("REC", L.cx - 20, L.top + 20, theme::kCrit, typeface::body(), textdatum_t::top_left);

    const uint32_t ms = elapsedMs(ctx);
    g.str(clock(ms).c_str(), L.cx, L.cy - 22, theme::kHi, typeface::title(),
          textdatum_t::middle_center);

    const int meterW = L.w - 32;
    const int meterY = L.cy + 4;
    int len = meterW * level_ / 900;
    if (len > meterW) len = meterW;
    g.c().drawRect(16, meterY, meterW, 8, theme::kDim);
    if (len > 2) g.c().fillRect(17, meterY + 1, len - 2, 6, theme::kHi);

    const uint32_t total = maxMs(ctx);
    const std::string left = clock(total > ms ? total - ms : 0) + " left";
    g.str(left.c_str(), L.cx, meterY + 18, theme::kDim, typeface::micro(),
          textdatum_t::top_center);

    widgets::hints(g, "DONE", "CANCEL");
  }

  void renderBusy(Gfx& g, ShellContext& ctx, const widgets::Layout& L, const char* verb) {
    if (ctx.character) {
      ctx.character->draw(g, L.cx, L.cy - 18, 44, MascotState{Expr::Think}, now_);
    }
    const std::string msg = std::string(verb) + dots();
    g.str(msg.c_str(), L.cx, L.cy + 16, theme::kFg, typeface::body(), textdatum_t::top_center);
    widgets::hints(g, nullptr, nullptr);
  }

  void renderThinking(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    int y = L.top + 22;
    y = widgets::wrapText(g, ctx.state.voice.transcript.c_str(), L.cx, y, L.w - 16,
                          typeface::micro(), theme::kDim, 11);
    if (ctx.character) {
      const int cy = (y + L.bottom) / 2;
      ctx.character->draw(g, L.cx, cy - 8, 36, MascotState{Expr::Think, 6, true, 0, false}, now_);
      const std::string msg = "thinking" + dots();
      g.str(msg.c_str(), L.cx, cy + 20, theme::kFg, typeface::body(), textdatum_t::top_center);
    }
    widgets::hints(g, nullptr, nullptr);
  }

  void renderReply(Gfx& g, ShellContext& ctx, const widgets::Layout& L) {
    std::vector<std::string> lines;
    wrapLines(g, ctx.state.voice.reply, L.w - 12, typeface::micro(), lines);

    const int top = L.top + 22;
    const int visible = (L.bottom - top - 4) / kLineH;
    overflow_ = static_cast<int>(lines.size()) > visible
                    ? static_cast<int>(lines.size()) - visible
                    : 0;
    if (scroll_ > overflow_) scroll_ = overflow_;

    int y = top;
    for (int i = scroll_; i < static_cast<int>(lines.size()) && i < scroll_ + visible; ++i) {
      g.str(lines[i].c_str(), 6, y, theme::kFg, typeface::micro(), textdatum_t::top_left);
      y += kLineH;
    }
    if (!ctx.state.voice.reply_done) {
      g.str(dots().c_str(), L.cx, L.bottom - 10, theme::kDim, typeface::body(),
            textdatum_t::top_center);
    }
    widgets::hints(g, "ASK", overflow_ > 0 ? "SCROLL" : "BACK");
  }

  IMicSource* mic_ = nullptr;
  bool recording_ = false;
  uint64_t samples_ = 0;
  int level_ = 0;
  int scroll_ = 0;
  int overflow_ = 0;
  uint32_t now_ = 0;
  AnimClock anim_;
};

}  // namespace

AppScreen& ask() {
  static AskScreen instance;
  return instance;
}

}  // namespace tama::screens

#endif  // TAMA_ENABLE_BUDDY
