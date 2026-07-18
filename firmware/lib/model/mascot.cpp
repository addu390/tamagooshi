#include "mascot.h"

#include "device.h"
#include "pet.h"

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

const char* moodFace(Mood m) {
  switch (m) {
    case Mood::Happy: return "( ^_^ )";
    case Mood::Neutral: return "( -_- )";
    case Mood::Sick: return "( x_x )";
    case Mood::Panic: return "(>_<)!";
    case Mood::Celebrate: return "\\(^o^)/";
    case Mood::Sleepy: return "( u_u )";
    default: return "( ?_? )";
  }
}

ExpressionKind expressionKindFromString(const std::string& s) {
  if (s == "chirp") return ExpressionKind::Chirp;
  if (s == "celebrate") return ExpressionKind::Celebrate;
  if (s == "haptic") return ExpressionKind::Haptic;
  if (s == "blink") return ExpressionKind::Blink;
  return ExpressionKind::Unknown;
}

const char* expressionKindToString(ExpressionKind k) {
  switch (k) {
    case ExpressionKind::Chirp: return "chirp";
    case ExpressionKind::Celebrate: return "celebrate";
    case ExpressionKind::Haptic: return "haptic";
    case ExpressionKind::Blink: return "blink";
    default: return "unknown";
  }
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

Expr exprFromPet(const PetState& pet) {
  if (pet.care < 22 || pet.energy < 18) return Expr::Worried;
  if (pet.energy < 38) return Expr::Sleepy;
  if (pet.bond > 68 && pet.care > 58) return Expr::Happy;
  return Expr::Neutral;
}

MascotState deriveMascot(const DeviceState& state, bool promptActive) {
  if (promptActive) return {Expr::Alert};
  if (state.voice_active) return {Expr::Think};
  return {exprFromMood(state.mood)};
}

}  // namespace tama
