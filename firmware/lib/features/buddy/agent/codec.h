#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "buddy/controller.h"

namespace tama {

enum class AgentMsg { None, Snapshot, Turn, Time, Owner, Command, Transcript, Reply, Agents };

struct AgentInbound {
  AgentMsg kind = AgentMsg::None;
  BuddySnapshot snapshot;
  std::string turn_text;
  int64_t epoch = 0;
  int tz_offset = 0;
  std::string owner;
  std::string command;
  std::string name;
  std::string voice_id;
  std::string voice_text;
  std::string voice_agent;
  std::vector<std::string> agent_list;
  std::string agent_default;
  bool reply_done = false;
};

struct AgentStatus {
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

class AgentCodec {
 public:
  bool decode(const std::string& line, AgentInbound& out) const;

  std::string encodePermission(const std::string& id, bool allow) const;
  std::string encodeAgentsRequest() const;
  std::string encodeAck(const std::string& cmd, bool ok, long n = 0) const;
  std::string encodeStatus(const AgentStatus& status) const;
};

}  // namespace tama
