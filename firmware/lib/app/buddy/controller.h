#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model.h"

namespace tama {

struct BuddySnapshot {
  int total = 0;
  int running = 0;
  int waiting = 0;
  std::string msg;
  std::vector<std::string> entries;
  uint64_t tokens = 0;
  uint64_t tokens_today = 0;
  bool has_prompt = false;
  std::string prompt_id;
  std::string prompt_tool;
  std::string prompt_hint;
};

class BuddyController {
 public:
  explicit BuddyController(DeviceState& state);

  void applySnapshot(const BuddySnapshot& snap, uint32_t nowMs);
  void applyTurn(const std::string& text, uint32_t nowMs);
  void setOwner(const std::string& name);
  void setConnected(bool connected, uint32_t nowMs);
  void recordDecision(bool allow);
  void tick(uint32_t nowMs);

 private:
  bool stale(uint32_t nowMs) const;
  bool ownsMood() const;
  void derivePhase(uint32_t nowMs);
  void syncPrompt();
  void applyMood();

  DeviceState& state_;
  bool connected_ = false;
  int prev_running_ = 0;
  uint32_t done_until_ms_ = 0;
};

}  // namespace tama
