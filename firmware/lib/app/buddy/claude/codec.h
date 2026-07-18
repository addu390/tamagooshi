#pragma once

#include <cstdint>
#include <string>

#include "buddy/controller.h"

namespace tama {

enum class ClaudeMsg { None, Snapshot, Turn, Time, Owner, Command };

struct ClaudeInbound {
  ClaudeMsg kind = ClaudeMsg::None;
  BuddySnapshot snapshot;
  std::string turn_text;
  int64_t epoch = 0;
  int tz_offset = 0;
  std::string owner;
  std::string command;
  std::string name;
};

struct ClaudeStatus {
  std::string name;
  bool secured = false;
  int batt_pct = 0;
  int mV = 0;
  int mA = 0;
  bool usb = false;
  uint32_t up_secs = 0;
  uint32_t heap = 0;
  int approvals = 0;
  int denials = 0;
};

class ClaudeCodec {
 public:
  bool decode(const std::string& line, ClaudeInbound& out) const;

  std::string encodePermission(const std::string& id, bool allow) const;
  std::string encodeAck(const std::string& cmd, bool ok, long n = 0) const;
  std::string encodeStatus(const ClaudeStatus& status) const;
};

}  // namespace tama
