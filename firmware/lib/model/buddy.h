#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tama {

enum class BuddyPhase { Offline, Idle, Working, Waiting, Done };

struct BuddyPrompt {
  bool active = false;
  std::string id;
  std::string tool;
  std::string hint;
};

struct BuddyState {
  BuddyPhase phase = BuddyPhase::Offline;
  int total = 0;
  int running = 0;
  int waiting = 0;
  std::string msg;
  std::vector<std::string> entries;
  uint64_t tokens = 0;
  uint64_t tokens_today = 0;
  BuddyPrompt prompt;
  std::string owner;
  uint32_t last_seen_ms = 0;
  int approvals = 0;
  int denials = 0;
};

const char* buddyPhaseLabel(BuddyPhase phase);

}  // namespace tama
