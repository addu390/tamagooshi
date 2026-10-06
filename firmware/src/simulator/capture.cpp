#include "capture.h"

#include <cstdio>
#include <cstdlib>

#include "runtime.h"

namespace tama::sim {

namespace {

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

CaptureHarness::CaptureHarness(Runtime& runtime) : runtime_(runtime) {}

void CaptureHarness::init() {
  if (const char* spec = std::getenv("TAMA_INPUT")) parseScript(spec);
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
