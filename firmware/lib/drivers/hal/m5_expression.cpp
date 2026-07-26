#include "m5_expression.h"

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <M5Unified.h>

namespace tama {

namespace {

constexpr int kAlertToneHz = 2000;
constexpr uint32_t kAlertToneMs = 400;
constexpr int kBaseVolume = 160;
constexpr uint32_t kPulseBedPeriodMs = 600;
constexpr uint32_t kSoftBedPeriodMs = 900;

uint32_t scaleMs(uint32_t baseMs, int intensityPct) {
  if (intensityPct <= 0) return 0;
  if (intensityPct >= 100) return baseMs;
  return baseMs * static_cast<uint32_t>(intensityPct) / 100;
}

int scaleVolume(int intensityPct) {
  if (intensityPct <= 0) return 0;
  if (intensityPct >= 100) return kBaseVolume;
  return kBaseVolume * intensityPct / 100;
}

void toneAt(int hz, uint32_t ms, int intensityPct) {
  const uint32_t dur = scaleMs(ms, intensityPct);
  if (dur == 0) return;
  M5.Speaker.setVolume(scaleVolume(intensityPct));
  M5.Speaker.tone(hz, dur);
}

void toneFor(const ExpressionCue& cue) {
  const int pct = cue.intensityPct;
  switch (cue.kind) {
    case ExpressionKind::Chirp:
      toneAt(3200, cue.durationMs ? cue.durationMs : 90, pct);
      break;
    case ExpressionKind::Celebrate:
      toneAt(2600, cue.durationMs ? cue.durationMs : 120, pct);
      break;
    case ExpressionKind::Haptic:
      toneAt(180, cue.durationMs ? cue.durationMs : 200, pct);
      break;
    case ExpressionKind::Tick:
      toneAt(1400, cue.durationMs ? cue.durationMs : 40, pct);
      break;
    case ExpressionKind::Warn:
      toneAt(420, cue.durationMs ? cue.durationMs : 160, pct);
      break;
    case ExpressionKind::Blink:
    default:
      break;
  }
}

void pulseBed(ExpressionBed bed) {
  switch (bed) {
    case ExpressionBed::Pulse:
      toneAt(1400, 40, 45);
      break;
    case ExpressionBed::Soft:
      toneAt(900, 30, 35);
      break;
    default:
      break;
  }
}

uint32_t bedPeriodMs(ExpressionBed bed) {
  switch (bed) {
    case ExpressionBed::Pulse: return kPulseBedPeriodMs;
    case ExpressionBed::Soft: return kSoftBedPeriodMs;
    default: return 0;
  }
}

ExpressionBed activeBed = ExpressionBed::None;
uint32_t lastBedMs = 0;
uint32_t ledUntilMs = 0;

}  // namespace

M5Expression::M5Expression(int redLedPin) : redLedPin_(redLedPin) {}

void M5Expression::begin() {
#ifdef ARDUINO
  if (redLedPin_ >= 0) pinMode(redLedPin_, OUTPUT);
#endif
  led(false);
  M5.Speaker.begin();
  M5.Speaker.setVolume(kBaseVolume);
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
  const uint32_t now = m5gfx::millis();
  const bool wantBuzz = state.buzzerActive && !state.muted && M5.Speaker.isRunning();
  if (wantBuzz && !buzzing_) {
    M5.Speaker.setVolume(kBaseVolume);
    M5.Speaker.tone(kAlertToneHz, kAlertToneMs);
  } else if (!wantBuzz && buzzing_) {
    M5.Speaker.stop();
  }
  buzzing_ = wantBuzz;

  const bool wantLed = state.alertActive || (ledUntilMs != 0 && now < ledUntilMs);
  if (!wantLed) ledUntilMs = 0;
  if (wantLed != ledOn_) led(wantLed);

  if (state.muted || state.bed == ExpressionBed::None || wantBuzz || !M5.Speaker.isRunning()) {
    activeBed = ExpressionBed::None;
    return;
  }

  const uint32_t period = bedPeriodMs(state.bed);
  if (period == 0) {
    activeBed = ExpressionBed::None;
    return;
  }

  if (state.bed != activeBed) {
    activeBed = state.bed;
    lastBedMs = now;
    pulseBed(state.bed);
  } else if (now - lastBedMs >= period) {
    lastBedMs = now;
    pulseBed(state.bed);
  }
}

void M5Expression::play(const ExpressionCue& cue) {
  if (cue.kind == ExpressionKind::Blink) {
    led(true);
    ledUntilMs = cue.durationMs ? m5gfx::millis() + cue.durationMs : 0;
    return;
  }
  if (!M5.Speaker.isRunning()) return;
  toneFor(cue);
}

}  // namespace tama
