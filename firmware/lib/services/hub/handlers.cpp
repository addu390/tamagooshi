#include "handlers.h"

#include "theme.h"
#include "types.h"

namespace tama {

void HandlerSet::bind(MessageRouter& router) {
  router.on(mtype::kBrandingSet, branding);
  router.on(mtype::kMetricUpsert, metric);
  router.on(mtype::kMoodSet, mood);
  router.on(mtype::kConfigSet, config);
  router.on(mtype::kTimeSet, time);
  router.on(mtype::kPageRaise, pageRaise);
  router.on(mtype::kPageClear, pageClear);
  router.on(mtype::kExpressionPlay, expression);
  router.on(mtype::kVoiceSessionStart, voiceStart);
  router.on(mtype::kVoiceSessionStop, voiceStop);
  router.on(mtype::kCommandExec, command);
}

void BrandingHandler::handle(const Envelope& env) {
  if (codec_.parseBranding(env.body, state_.branding)) state_.dirty = true;
}

void MetricHandler::handle(const Envelope& env) {
  Metric metric;
  if (codec_.parseMetric(env.body, metric)) {
    state_.upsertMetric(metric);
    state_.dirty = true;
  }
}

void MoodHandler::handle(const Envelope& env) {
  if (codec_.parseMood(env.body, state_.mood, state_.mood_reason)) state_.dirty = true;
}

void ConfigHandler::handle(const Envelope& env) {
  ConfigPatch patch;
  if (!codec_.parseConfig(env.body, patch)) return;
  if (patch.carousel_secs) state_.carousel_secs = *patch.carousel_secs;
  if (patch.buzzer_enabled) state_.buzzer_enabled = *patch.buzzer_enabled;
  if (!patch.theme.empty()) theme::setThemeByName(patch.theme.c_str());
  if (!patch.character_id.empty()) state_.character_id = patch.character_id;
  state_.dirty = true;
}

void TimeHandler::handle(const Envelope& env) {
  int64_t epoch = 0;
  int tzOffsetMin = 0;
  if (!codec_.parseTime(env.body, epoch, tzOffsetMin)) return;
  state_.tz_offset_min = static_cast<int16_t>(tzOffsetMin);
  if (epoch > 0) system_.setClock(epoch);
  state_.dirty = true;
}

void PageRaiseHandler::handle(const Envelope& env) {
  Page page;
  if (!codec_.parsePage(env.body, page)) return;
  if (page.source.empty()) page.source = "hub";
  if (page.actionCount == 0) {
    page.actions[0] = {"ACK", PromptOutcome::Ack};
    page.actions[1] = {"SNOOZE", PromptOutcome::Snooze};
    page.actionCount = 2;
  }
  state_.raisePrompt(page);
}

void PageClearHandler::handle(const Envelope& env) {
  std::string id;
  codec_.parsePageRef(env.body, id);
  if (id.empty()) {
    state_.clearPromptSource("hub");
  } else {
    state_.clearPrompt(id);
  }
}

void ExpressionHandler::handle(const Envelope& env) {
  ExpressionCue cue;
  if (codec_.parseExpression(env.body, cue)) sink_.play(cue);
}

void VoiceStartHandler::handle(const Envelope&) {
  state_.voice_active = true;
  sink_.play(ExpressionCue{ExpressionKind::Chirp, 100, 0});
  state_.dirty = true;
}

void VoiceStopHandler::handle(const Envelope&) {
  state_.voice_active = false;
  state_.dirty = true;
}

void CommandHandler::handle(const Envelope& env) {
  std::string cmd;
  if (!codec_.parseCommand(env.body, cmd)) return;
  if (cmd == "reboot") {
    system_.reboot();
  } else if (cmd == "identify") {
    sink_.play(ExpressionCue{ExpressionKind::Blink, 100, 1500});
  }
}

}  // namespace tama
