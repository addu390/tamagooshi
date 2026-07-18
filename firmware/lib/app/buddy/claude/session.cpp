#include "session.h"

namespace tama {

ClaudeSession::ClaudeSession(ILineSink& sink, BuddyController& controller, ClaudeCommands& commands)
    : sink_(sink), controller_(controller), commands_(commands) {}

void ClaudeSession::onConnected(bool connected) { controller_.setConnected(connected, now_ms_); }

void ClaudeSession::onInbound(const std::string&, const std::string& payload) {
  ClaudeInbound in;
  if (!codec_.decode(payload, in)) return;
  switch (in.kind) {
    case ClaudeMsg::Snapshot: controller_.applySnapshot(in.snapshot, now_ms_); break;
    case ClaudeMsg::Turn: controller_.applyTurn(in.turn_text, now_ms_); break;
    case ClaudeMsg::Owner:
      controller_.setOwner(in.owner);
      sink_.send(codec_.encodeAck("owner", true));
      break;
    case ClaudeMsg::Command: {
      const std::string reply = commands_.handle(in.command);
      if (!reply.empty()) sink_.send(reply);
      break;
    }
    case ClaudeMsg::Time:
    case ClaudeMsg::None:
    default: break;
  }
}

void ClaudeSession::tick(uint32_t nowMs) {
  now_ms_ = nowMs;
  controller_.tick(nowMs);
}

void ClaudeSession::decide(const std::string& id, bool allow) {
  sink_.send(codec_.encodePermission(id, allow));
  controller_.recordDecision(allow);
}

}  // namespace tama
