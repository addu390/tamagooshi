#include "controller.h"

namespace tama {

namespace {
constexpr uint32_t kStaleMs = 30000;
constexpr uint32_t kDoneMs = 5000;
constexpr size_t kMaxEntries = 4;
}  // namespace

BuddyController::BuddyController(DeviceState& state) : state_(state) {}

void BuddyController::applySnapshot(const BuddySnapshot& snap, uint32_t nowMs) {
  BuddyState& b = state_.buddy;
  b.total = snap.total;
  b.running = snap.running;
  b.waiting = snap.waiting;
  b.msg = snap.msg;
  b.tokens = snap.tokens;
  b.tokens_today = snap.tokens_today;
  b.last_seen_ms = nowMs;

  b.entries = snap.entries;
  if (b.entries.size() > kMaxEntries) b.entries.resize(kMaxEntries);

  b.prompt.active = snap.has_prompt;
  b.prompt.id = snap.prompt_id;
  b.prompt.tool = snap.prompt_tool;
  b.prompt.hint = snap.prompt_hint;

  connected_ = true;
  derivePhase(nowMs);
  syncPrompt();
  applyMood();
  state_.dirty = true;
}

void BuddyController::applyTurn(const std::string& text, uint32_t nowMs) {
  if (text.empty()) return;
  BuddyState& b = state_.buddy;
  b.last_seen_ms = nowMs;
  b.entries.insert(b.entries.begin(), text);
  if (b.entries.size() > kMaxEntries) b.entries.resize(kMaxEntries);
  state_.dirty = true;
}

void BuddyController::setOwner(const std::string& name) {
  state_.buddy.owner = name;
  state_.dirty = true;
}

void BuddyController::setConnected(bool connected, uint32_t nowMs) {
  connected_ = connected;
  if (connected) state_.buddy.last_seen_ms = nowMs;
  derivePhase(nowMs);
  applyMood();
  state_.dirty = true;
}

void BuddyController::recordDecision(bool allow) {
  if (allow) {
    ++state_.buddy.approvals;
  } else {
    ++state_.buddy.denials;
  }
}

void BuddyController::tick(uint32_t nowMs) {
  const BuddyPhase before = state_.buddy.phase;
  derivePhase(nowMs);
  if (state_.buddy.phase != before) {
    applyMood();
    state_.dirty = true;
  }
}

bool BuddyController::stale(uint32_t nowMs) const {
  return nowMs - state_.buddy.last_seen_ms > kStaleMs;
}

void BuddyController::derivePhase(uint32_t nowMs) {
  BuddyState& b = state_.buddy;

  if (prev_running_ > 0 && b.running == 0 && b.waiting == 0 && b.total > 0) {
    done_until_ms_ = nowMs + kDoneMs;
  }
  prev_running_ = b.running;

  if (!connected_ || stale(nowMs)) {
    b.phase = BuddyPhase::Offline;
  } else if (b.waiting > 0) {
    b.phase = BuddyPhase::Waiting;
  } else if (b.running > 0) {
    b.phase = BuddyPhase::Working;
  } else if (nowMs < done_until_ms_) {
    b.phase = BuddyPhase::Done;
  } else {
    b.phase = BuddyPhase::Idle;
  }
}

void BuddyController::syncPrompt() {
  const BuddyPrompt& p = state_.buddy.prompt;
  if (!p.active) {
    state_.clearPromptSource("claude");
    return;
  }
  Page page;
  page.id = p.id;
  page.severity = Severity::Warning;
  page.title = p.tool.empty() ? "permission" : p.tool;
  page.body = p.hint;
  page.source = "claude";
  page.requires_ack = true;
  page.actions[0] = {"ALLOW", PromptOutcome::Allow};
  page.actions[1] = {"DENY", PromptOutcome::Deny};
  page.actionCount = 2;
  state_.raisePrompt(page);
}

bool BuddyController::ownsMood() const {
  if (!connected_) return false;
  if (!state_.hub_connected) return true;
  const BuddyPhase phase = state_.buddy.phase;
  return phase == BuddyPhase::Working || phase == BuddyPhase::Waiting || phase == BuddyPhase::Done;
}

void BuddyController::applyMood() {
  if (!ownsMood()) return;
  switch (state_.buddy.phase) {
    case BuddyPhase::Waiting: state_.mood = Mood::Panic; break;
    case BuddyPhase::Done: state_.mood = Mood::Celebrate; break;
    case BuddyPhase::Working: state_.mood = Mood::Neutral; break;
    case BuddyPhase::Idle: state_.mood = Mood::Happy; break;
    case BuddyPhase::Offline:
    default: state_.mood = Mood::Sleepy; break;
  }
}

}  // namespace tama
