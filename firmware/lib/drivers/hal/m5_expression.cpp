#include "m5_expression.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <M5Unified.h>

namespace tama {

namespace {

constexpr int kAlertToneHz = 2000;
constexpr uint32_t kAlertToneMs = 400;

void toneFor(const ExpressionCue& cue) {
  switch (cue.kind) {
    case ExpressionKind::Chirp:
      M5.Speaker.tone(3200, 90);
      break;
    case ExpressionKind::Celebrate:
      M5.Speaker.tone(2600, 120);
      break;
    case ExpressionKind::Haptic:
      M5.Speaker.tone(180, cue.durationMs ? cue.durationMs : 200);
      break;
    case ExpressionKind::Blink:
    default:
      break;
  }
}

}  // namespace

M5Expression::M5Expression(int redLedPin) : redLedPin_(redLedPin) {}

void M5Expression::begin() {
#ifdef ARDUINO
  if (redLedPin_ >= 0) pinMode(redLedPin_, OUTPUT);
#endif
  led(false);
  M5.Speaker.begin();
  M5.Speaker.setVolume(160);
}

void M5Expression::led(bool on) {
  ledOn_ = on;
#ifdef ARDUINO
  if (redLedPin_ >= 0) digitalWrite(redLedPin_, on ? LOW : HIGH);
#else
  (void)redLedPin_;
#endif
}

void M5Expression::apply(const ExpressionState& state) {
  const bool wantBuzz = state.buzzerActive && !state.muted && M5.Speaker.isRunning();
  if (wantBuzz && !buzzing_) {
    M5.Speaker.tone(kAlertToneHz, kAlertToneMs);
  } else if (!wantBuzz && buzzing_) {
    M5.Speaker.stop();
  }
  buzzing_ = wantBuzz;

  const bool wantLed = state.alertActive;
  if (wantLed != ledOn_) led(wantLed);
}

void M5Expression::play(const ExpressionCue& cue) {
  if (cue.kind == ExpressionKind::Blink) {
    led(true);
    return;
  }
  if (!M5.Speaker.isRunning()) return;
  toneFor(cue);
}

}  // namespace tama
