#include "capture.h"

#include <cstdio>
#include <cstdlib>

#include "buddy/claude/session.h"
#include "runtime.h"

namespace tama::sim {

namespace {

constexpr const char* kOwner = "{\"cmd\":\"owner\",\"name\":\"John\"}";

int envInt(const char* key, int fallback) {
  const char* v = std::getenv(key);
  return v ? std::atoi(v) : fallback;
}

Intent intentFromName(const std::string& s) {
  if (s == "next") return Intent::Next;
  if (s == "prev") return Intent::Prev;
  if (s == "back") return Intent::Back;
  if (s == "home") return Intent::Home;
  return Intent::Select;
}

}  // namespace

CaptureHarness::CaptureHarness(Runtime& runtime, ClaudeSession& session)
    : runtime_(runtime), session_(session) {}

void CaptureHarness::init() {
  if (const char* spec = std::getenv("TAMA_INPUT")) parseScript(spec);
  if (const char* mode = std::getenv("TAMA_BUDDY")) loadBuddyArc(mode);
  if (const char* dir = std::getenv("TAMA_CAP_DIR")) {
    capDir_ = dir;
    warmup_ = envInt("TAMA_CAP_WARMUP", 24);
    stride_ = envInt("TAMA_CAP_STRIDE", 8);
    total_ = envInt("TAMA_CAP_N", 75);
  }
}

void CaptureHarness::beforeFrame(uint32_t nowMs) {
  for (auto& step : script_) {
    if (!step.fired && nowMs >= step.at) {
      step.fired = true;
      runtime_.nav().dispatch(step.intent);
    }
  }

  if (!buddy_.empty()) {
    session_.tick(nowMs);
    for (auto& step : buddy_) {
      if (!step.fired && nowMs >= step.at) {
        step.fired = true;
        session_.onInbound("", step.json);
      }
    }
  }

  armed_ = false;
  if (capDir_.empty()) return;
  const int idx = loops_++;
  if (idx < warmup_ || (idx - warmup_) % stride_ != 0) return;
  if (captured_ >= total_) std::exit(0);
  armed_ = true;
}

void CaptureHarness::afterFrame() {
  if (const char* path = std::getenv("TAMA_DUMP")) {
    if (++dumpFrames_ == 40) {
      writePPM(path);
      std::exit(0);
    }
  }
  if (!armed_) return;
  char path[512];
  std::snprintf(path, sizeof(path), "%s/frame_%04d.ppm", capDir_.c_str(), captured_);
  writePPM(path);
  ++captured_;
}

void CaptureHarness::parseScript(const std::string& spec) {
  size_t i = 0;
  while (i <= spec.size()) {
    const size_t comma = spec.find(',', i);
    const std::string tok = spec.substr(i, comma == std::string::npos ? comma : comma - i);
    const size_t colon = tok.find(':');
    if (colon != std::string::npos) {
      const uint32_t at = static_cast<uint32_t>(std::atoi(tok.substr(0, colon).c_str()));
      script_.push_back({at, intentFromName(tok.substr(colon + 1)), false});
    }
    if (comma == std::string::npos) break;
    i = comma + 1;
  }
}

void CaptureHarness::loadBuddyArc(const std::string& mode) {
  if (mode != "work" && mode != "approve") return;
  session_.onInbound("", kOwner);
  if (mode == "work") {
    buddy_ = {
        {0, "{\"total\":0,\"running\":0,\"waiting\":0,\"tokens\":0,\"tokens_today\":96000}", false},
        {2200,
         "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"reading repo\",\"entries\":[\"10:39 "
         "scan project\"],\"tokens\":12400,\"tokens_today\":108400}",
         false},
        {5200,
         "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"editing files\",\"entries\":[\"10:41 "
         "edit home.cpp\",\"10:39 scan project\"],\"tokens\":24800,\"tokens_today\":120800}",
         false},
        {9200,
         "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"running tests\",\"entries\":[\"10:43 "
         "yarn test\",\"10:41 edit home.cpp\",\"10:39 scan project\"],\"tokens\":33600,\"tokens_"
         "today\":129600}",
         false},
        {12200,
         "{\"total\":3,\"running\":0,\"waiting\":0,\"msg\":\"task complete\",\"entries\":[\"10:44 "
         "git push\",\"10:43 yarn test\",\"10:41 edit home.cpp\"],\"tokens\":41200,\"tokens_today\":"
         "137200}",
         false},
    };
    return;
  }
  buddy_ = {
      {0,
       "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"editing files\",\"entries\":[\"10:39 edit "
       "auth.ts\"],\"tokens\":18200,\"tokens_today\":102000}",
       false},
      {2000,
       "{\"total\":3,\"running\":1,\"waiting\":1,\"msg\":\"approve: Bash\",\"entries\":[\"10:39 edit "
       "auth.ts\"],\"tokens\":21600,\"tokens_today\":105400,\"prompt\":{\"id\":\"req_push\",\"tool\":"
       "\"Bash\",\"hint\":\"git push origin main\"}}",
       false},
      {5000,
       "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"pushed to main\",\"entries\":[\"10:41 git "
       "push\",\"10:39 edit auth.ts\"],\"tokens\":26400,\"tokens_today\":110200}",
       false},
      {6500,
       "{\"total\":3,\"running\":1,\"waiting\":1,\"msg\":\"approve: Bash\",\"entries\":[\"10:41 git "
       "push\",\"10:39 edit auth.ts\"],\"tokens\":28800,\"tokens_today\":112600,\"prompt\":{\"id\":"
       "\"req_rm\",\"tool\":\"Bash\",\"hint\":\"rm -rf build/\"}}",
       false},
      {9800,
       "{\"total\":3,\"running\":1,\"waiting\":0,\"msg\":\"skipped cleanup\",\"entries\":[\"10:43 "
       "denied rm -rf\",\"10:41 git push\",\"10:39 edit auth.ts\"],\"tokens\":31200,\"tokens_today\":"
       "115000}",
       false},
  };
}

void CaptureHarness::writePPM(const char* path) const {
  auto& gfx = runtime_.gfx();
  const int w = gfx.w();
  const int h = gfx.h();
  FILE* f = std::fopen(path, "wb");
  if (!f) return;
  std::fprintf(f, "P6\n%d %d\n255\n", w, h);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const auto c = gfx.c().readPixelRGB(x, y);
      const uint8_t px[3] = {c.r, c.g, c.b};
      std::fwrite(px, 1, 3, f);
    }
  }
  std::fclose(f);
}

}  // namespace tama::sim
