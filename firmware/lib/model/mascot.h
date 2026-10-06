#pragma once

#include <cstdint>
#include <string>

namespace tama {

struct DeviceState;

enum class Mood { Happy, Neutral, Sick, Panic, Celebrate, Sleepy, Unknown };

Mood moodFromString(const std::string& s);
const char* moodToString(Mood m);

enum class ExpressionKind { Chirp, Celebrate, Haptic, Blink, Tick, Warn, Unknown };

ExpressionKind expressionKindFromString(const std::string& s);

struct ExpressionCue {
  ExpressionKind kind = ExpressionKind::Chirp;
  int intensityPct = 100;
  uint32_t durationMs = 0;
};

enum class ExpressionBed { None, Pulse, Soft };

struct ExpressionState {
  bool alertActive = false;
  bool buzzerActive = false;
  bool muted = false;
  ExpressionBed bed = ExpressionBed::None;
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
MascotState deriveMascot(const DeviceState& state, bool promptActive);

}  // namespace tama
