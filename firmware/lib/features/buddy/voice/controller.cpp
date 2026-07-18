#include "controller.h"

namespace tama {

VoiceController::VoiceController(DeviceState& state) : state_(state) {}

void VoiceController::applyAgents(const std::vector<std::string>& agents,
                                  const std::string& preferred) {
  state_.agents.set(agents, preferred);
  state_.dirty = true;
}

void VoiceController::applyTranscript(const std::string& id, const std::string& text,
                                      const std::string& agent) {
  VoiceChat& v = state_.voice;
  v.phase = VoicePhase::Confirming;
  v.transcript = text;
  v.transcript_id = id;
  v.agent = agent;
  v.reply.clear();
  v.reply_done = false;

  Page page;
  page.id = id;
  page.severity = Severity::Info;
  page.title = "SEND PROMPT?";
  page.body = text;
  page.source = "voice";
  page.requires_ack = true;
  page.actions[0] = {"SEND", PromptOutcome::Allow};
  page.actions[1] = {"CANCEL", PromptOutcome::Deny};
  page.actionCount = 2;
  state_.raisePrompt(page);
  state_.dirty = true;
}

void VoiceController::applyReply(const std::string& text, bool done) {
  VoiceChat& v = state_.voice;
  v.phase = VoicePhase::Reply;
  v.reply += text;
  v.reply_done = done;
  state_.dirty = true;
}

void VoiceController::onDecision(const std::string& id, bool send) {
  VoiceChat& v = state_.voice;
  if (id != v.transcript_id) return;
  state_.clearPromptSource("voice");
  if (send) {
    v.phase = VoicePhase::Thinking;
  } else {
    v.reset();
  }
  state_.dirty = true;
}

void VoiceController::onConnected(bool connected) {
  if (connected) return;
  state_.clearPromptSource("voice");
  state_.voice.reset();
  state_.agents.clear();
  state_.dirty = true;
}

}  // namespace tama
