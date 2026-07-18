#include "session.h"

namespace tama {

AgentSession::AgentSession(ILineSink& sink, BuddyController& controller, AgentCommands& commands,
                           VoiceController& voice, VoiceUplink& uplink)
    : sink_(sink), controller_(controller), commands_(commands), voice_(voice), uplink_(uplink) {}

void AgentSession::onConnected(bool connected) {
  controller_.setConnected(connected, now_ms_);
  voice_.onConnected(connected);
  if (connected) {
    sink_.send(codec_.encodeAgentsRequest());
  } else {
    uplink_.cancel();
  }
}

void AgentSession::onInbound(const std::string&, const std::string& payload) {
  AgentInbound in;
  if (!codec_.decode(payload, in)) return;
  switch (in.kind) {
    case AgentMsg::Snapshot: controller_.applySnapshot(in.snapshot, now_ms_); break;
    case AgentMsg::Turn: controller_.applyTurn(in.turn_text, now_ms_); break;
    case AgentMsg::Owner:
      controller_.setOwner(in.owner);
      sink_.send(codec_.encodeAck("owner", true));
      break;
    case AgentMsg::Command: {
      const std::string reply = commands_.handle(in.command);
      if (!reply.empty()) sink_.send(reply);
      break;
    }
    case AgentMsg::Transcript:
      voice_.applyTranscript(in.voice_id, in.voice_text, in.voice_agent);
      break;
    case AgentMsg::Agents: voice_.applyAgents(in.agent_list, in.agent_default); break;
    case AgentMsg::Reply: voice_.applyReply(in.voice_text, in.reply_done); break;
    case AgentMsg::Time:
    case AgentMsg::None:
    default: break;
  }
}

void AgentSession::tick(uint32_t nowMs) {
  now_ms_ = nowMs;
  controller_.tick(nowMs);
  uplink_.pump(nowMs);
}

void AgentSession::decide(const std::string& id, bool allow) {
  sink_.send(codec_.encodePermission(id, allow));
  controller_.recordDecision(allow);
}

void AgentSession::decideVoice(const std::string& id, bool send) {
  voice_.onDecision(id, send);
  sink_.send(codec_.encodePermission(id, send));
}

std::function<void(const std::string&, PromptOutcome)> makeAgentResolver(AgentSession& session) {
  return [&session](const std::string& id, PromptOutcome outcome) {
    if (outcome == PromptOutcome::Allow)
      session.decide(id, true);
    else if (outcome == PromptOutcome::Deny)
      session.decide(id, false);
  };
}

std::function<void(const std::string&, PromptOutcome)> makeVoiceResolver(AgentSession& session) {
  return [&session](const std::string& id, PromptOutcome outcome) {
    if (outcome == PromptOutcome::Allow)
      session.decideVoice(id, true);
    else if (outcome == PromptOutcome::Deny)
      session.decideVoice(id, false);
  };
}

}  // namespace tama
