#include "mascot.h"

#include "device.h"

namespace tama {

Mood moodFromString(const std::string& s) {
  if (s == "happy") return Mood::Happy;
  if (s == "neutral") return Mood::Neutral;
  if (s == "sick") return Mood::Sick;
  if (s == "panic") return Mood::Panic;
  if (s == "celebrate") return Mood::Celebrate;
  if (s == "sleepy") return Mood::Sleepy;
  return Mood::Unknown;
}

const char* moodToString(Mood m) {
  switch (m) {
    case Mood::Happy: return "happy";
    case Mood::Neutral: return "neutral";
    case Mood::Sick: return "sick";
    case Mood::Panic: return "panic";
    case Mood::Celebrate: return "celebrate";
    case Mood::Sleepy: return "sleepy";
    default: return "unknown";
  }
}

ExpressionKind expressionKindFromString(const std::string& s) {
  if (s == "chirp") return ExpressionKind::Chirp;
  if (s == "celebrate") return ExpressionKind::Celebrate;
  if (s == "haptic") return ExpressionKind::Haptic;
  if (s == "blink") return ExpressionKind::Blink;
  if (s == "tick") return ExpressionKind::Tick;
  if (s == "warn") return ExpressionKind::Warn;
  return ExpressionKind::Unknown;
}

Expr exprFromMood(Mood m) {
  switch (m) {
    case Mood::Happy: return Expr::Happy;
    case Mood::Celebrate: return Expr::Celebrate;
    case Mood::Sleepy: return Expr::Sleepy;
    case Mood::Sick: return Expr::Worried;
    case Mood::Panic: return Expr::Alert;
    case Mood::Neutral: return Expr::Neutral;
    default: return Expr::Neutral;
  }
}

MascotState deriveMascot(const DeviceState& state, bool promptActive) {
  if (promptActive) return {Expr::Alert};
  if (state.voice_active) return {Expr::Think};
  return {exprFromMood(state.mood)};
}

}  // namespace tama
