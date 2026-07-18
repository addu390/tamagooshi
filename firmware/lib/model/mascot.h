#pragma once

#include <cstdint>
#include <string>

namespace tama {

struct PetState;
struct DeviceState;

enum class Mood { Happy, Neutral, Sick, Panic, Celebrate, Sleepy, Unknown };

Mood moodFromString(const std::string& s);
const char* moodToString(Mood m);
const char* moodFace(Mood m);

enum class ExpressionKind { Chirp, Celebrate, Haptic, Blink, Unknown };

ExpressionKind expressionKindFromString(const std::string& s);
const char* expressionKindToString(ExpressionKind k);

struct ExpressionCue {
  ExpressionKind kind = ExpressionKind::Chirp;
  int intensityPct = 100;
  uint32_t durationMs = 0;
};

struct ExpressionState {
  bool alertActive = false;
  bool buzzerActive = false;
  bool muted = false;
};

enum class Expr { Happy, Neutral, Sleepy, Think, Alert, Celebrate, Worried };

struct MascotState {
  Expr expr = Expr::Neutral;
  int wanderPx = 0;
  bool still = false;
  int liftPx = 0;
  bool shadow = true;
};

Expr exprFromMood(Mood m);
Expr exprFromPet(const PetState& pet);
MascotState deriveMascot(const DeviceState& state, bool promptActive);

}  // namespace tama
