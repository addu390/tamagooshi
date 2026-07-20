#include "hal/m5_ir.h"

#if TAMA_BOARD_HAS_IR

#include <M5Unified.h>
#include <Preferences.h>
#include <driver/rmt.h>

#include <algorithm>
#include <string>

namespace tama {

namespace {

// ESP32-S3 RMT allows TX only on channels 0-3 and RX only on 4-7.
constexpr rmt_channel_t kTxChannel = RMT_CHANNEL_0;
constexpr rmt_channel_t kRxChannel = RMT_CHANNEL_4;
constexpr uint8_t kClkDivToMicros = 80;
constexpr uint32_t kCarrierHz = 38000;
constexpr uint8_t kCarrierDutyPct = 33;
constexpr uint16_t kGlitchMicros = 100;
constexpr uint16_t kFrameGapMicros = 12000;
constexpr int kMinLearnedPulses = 12;

int appendPulse(IrFrame& frame, uint32_t micros) {
  if (micros == 0 || frame.count >= IrFrame::kMaxPulses) return 0;
  frame.pulses[frame.count++] =
      static_cast<uint16_t>(std::min<uint32_t>(micros, UINT16_MAX));
  return 1;
}

void toFrame(const rmt_item32_t* items, size_t count, IrFrame& out) {
  out.count = 0;
  for (size_t i = 0; i < count; ++i) {
    if (!appendPulse(out, items[i].duration0)) return;
    if (!appendPulse(out, items[i].duration1)) return;
  }
}

constexpr char kStoreNamespace[] = "remote";

std::string labelKey(int i) { return "label" + std::to_string(i); }
std::string frameKey(int i) { return "frame" + std::to_string(i); }

}  // namespace

void M5IrTransceiver::begin() {
  M5.Power.setExtOutput(true);

  rmt_config_t tx = RMT_DEFAULT_CONFIG_TX(static_cast<gpio_num_t>(txPin_), kTxChannel);
  tx.clk_div = kClkDivToMicros;
  tx.tx_config.carrier_en = true;
  tx.tx_config.carrier_freq_hz = kCarrierHz;
  tx.tx_config.carrier_duty_percent = kCarrierDutyPct;
  tx.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
  tx.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  tx.tx_config.idle_output_en = true;
  rmt_config(&tx);
  rmt_driver_install(kTxChannel, 0, 0);

  rmt_config_t rx = RMT_DEFAULT_CONFIG_RX(static_cast<gpio_num_t>(rxPin_), kRxChannel);
  rx.clk_div = kClkDivToMicros;
  rx.rx_config.filter_en = true;
  rx.rx_config.filter_ticks_thresh = kGlitchMicros;
  rx.rx_config.idle_threshold = kFrameGapMicros;
  rmt_config(&rx);
  rmt_driver_install(kRxChannel, 2048, 0);
  rmt_get_ringbuf_handle(kRxChannel, &rx_);
}

void M5IrTransceiver::send(const IrFrame& frame) {
  if (frame.empty()) return;
  rmt_item32_t items[IrFrame::kMaxPulses / 2 + 1] = {};
  size_t n = 0;
  for (int i = 0; i < frame.count; i += 2) {
    items[n].level0 = 1;
    items[n].duration0 = frame.pulses[i];
    items[n].level1 = 0;
    items[n].duration1 = i + 1 < frame.count ? frame.pulses[i + 1] : 0;
    ++n;
  }
  rmt_write_items(kTxChannel, items, n, true);
}

void M5IrTransceiver::startLearn() {
  if (learning_ || rx_ == nullptr) return;
  M5.Speaker.end();
  size_t len = 0;
  while (void* stale = xRingbufferReceive(rx_, &len, 0)) {
    vRingbufferReturnItem(rx_, stale);
  }
  rmt_rx_start(kRxChannel, true);
  learning_ = true;
}

void M5IrTransceiver::stopLearn() {
  if (!learning_) return;
  rmt_rx_stop(kRxChannel);
  M5.Speaker.begin();
  learning_ = false;
}

bool M5IrTransceiver::fetchLearned(IrFrame& out) {
  if (!learning_ || rx_ == nullptr) return false;
  size_t len = 0;
  auto* items = static_cast<rmt_item32_t*>(xRingbufferReceive(rx_, &len, 0));
  if (items == nullptr) return false;
  toFrame(items, len / sizeof(rmt_item32_t), out);
  vRingbufferReturnItem(rx_, items);
  return out.count >= kMinLearnedPulses;
}

int NvsIrStore::load(IrButton* out, int max) {
  Preferences prefs;
  if (!prefs.begin(kStoreNamespace, true)) return 0;
  const int count = std::min<int>(prefs.getUChar("count", 0), max);
  int n = 0;
  for (int i = 0; i < count; ++i) {
    IrButton& b = out[n];
    b.label = prefs.getString(labelKey(i).c_str(), "").c_str();
    const size_t bytes =
        prefs.getBytes(frameKey(i).c_str(), b.frame.pulses, sizeof(b.frame.pulses));
    b.frame.count = static_cast<uint8_t>(bytes / sizeof(b.frame.pulses[0]));
    if (!b.label.empty() && !b.frame.empty()) ++n;
  }
  prefs.end();
  return n;
}

void NvsIrStore::save(const IrButton* buttons, int count) {
  Preferences prefs;
  if (!prefs.begin(kStoreNamespace, false)) return;
  prefs.clear();
  prefs.putUChar("count", static_cast<uint8_t>(count));
  for (int i = 0; i < count; ++i) {
    prefs.putString(labelKey(i).c_str(), buttons[i].label.c_str());
    prefs.putBytes(frameKey(i).c_str(), buttons[i].frame.pulses,
                   buttons[i].frame.count * sizeof(buttons[i].frame.pulses[0]));
  }
  prefs.end();
}

}  // namespace tama

#endif  // TAMA_BOARD_HAS_IR
