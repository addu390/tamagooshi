#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "intent.h"

namespace tama {
class Runtime;
}  // namespace tama

namespace tama::sim {

class CaptureHarness {
 public:
  explicit CaptureHarness(Runtime& runtime);

  void init();
  void beforeFrame(uint32_t nowMs);
  void afterFrame();

 private:
  struct ScriptStep {
    uint32_t at;
    Intent intent;
    bool fired;
  };

  void parseScript(const std::string& spec);
  void writePPM(const char* path) const;

  Runtime& runtime_;

  std::vector<ScriptStep> script_;

  std::string capDir_;
  int warmup_ = 24;
  int stride_ = 8;
  int total_ = 75;
  int loops_ = 0;
  int captured_ = 0;
  bool armed_ = false;
  int dumpFrames_ = 0;
};

}  // namespace tama::sim
