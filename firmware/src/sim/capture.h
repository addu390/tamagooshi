#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "intent.h"

namespace tama {
class Runtime;
class AgentSession;
}  // namespace tama

namespace tama::sim {

class CaptureHarness {
 public:
  explicit CaptureHarness(Runtime& runtime, AgentSession* session = nullptr);

  void init();
  void beforeFrame(uint32_t nowMs);
  void afterFrame();

 private:
  struct ScriptStep {
    uint32_t at;
    Intent intent;
    bool fired;
  };
  struct BuddyStep {
    uint32_t at;
    std::string json;
    bool fired;
  };

  void parseScript(const std::string& spec);
  void loadBuddyArc(const std::string& mode);
  void writePPM(const char* path) const;

  Runtime& runtime_;
  AgentSession* session_;

  std::vector<ScriptStep> script_;
  std::vector<BuddyStep> buddy_;

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
