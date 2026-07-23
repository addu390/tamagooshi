#include "hal/m5_ir.h"

#if TAMA_BOARD_HAS_IR

#include <M5Unified.h>
#include <Preferences.h>

#include <algorithm>
#include <string>

namespace tama {

namespace {

constexpr uint32_t kResolutionHz = 1000000;
constexpr uint32_t kCarrierHz = 38000;
constexpr float kCarrierDuty = 0.33f;
constexpr uint32_t kGlitchNs = 100 * 1000;
constexpr uint32_t kFrameGapNs = 12000 * 1000;
constexpr size_t kMemBlockSymbols = 48;
constexpr int kMinLearnedPulses = 12;

int appendPulse(IrFrame& frame, uint32_t micros) {
  if (micros == 0 || frame.count >= IrFrame::kMaxPulses) return 0;
  frame.pulses[frame.count++] =
      static_cast<uint16_t>(std::min<uint32_t>(micros, UINT16_MAX));
  return 1;
}

void toFrame(const rmt_symbol_word_t* items, size_t count, IrFrame& out) {
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

  rmt_tx_channel_config_t tx = {};
  tx.gpio_num = static_cast<gpio_num_t>(txPin_);
  tx.clk_src = RMT_CLK_SRC_DEFAULT;
  tx.resolution_hz = kResolutionHz;
  tx.mem_block_symbols = kMemBlockSymbols;
  tx.trans_queue_depth = 4;
  if (rmt_new_tx_channel(&tx, &tx_) != ESP_OK) return;

  rmt_carrier_config_t carrier = {};
  carrier.frequency_hz = kCarrierHz;
  carrier.duty_cycle = kCarrierDuty;
  rmt_apply_carrier(tx_, &carrier);

  rmt_copy_encoder_config_t enc = {};
  rmt_new_copy_encoder(&enc, &encoder_);
  rmt_enable(tx_);

  rmt_rx_channel_config_t rx = {};
  rx.gpio_num = static_cast<gpio_num_t>(rxPin_);
  rx.clk_src = RMT_CLK_SRC_DEFAULT;
  rx.resolution_hz = kResolutionHz;
  rx.mem_block_symbols = kMemBlockSymbols;
  if (rmt_new_rx_channel(&rx, &rx_) != ESP_OK) return;

  rmt_rx_event_callbacks_t cbs = {};
  cbs.on_recv_done = &M5IrTransceiver::onRxDone;
  rmt_rx_register_event_callbacks(rx_, &cbs, this);
}

bool M5IrTransceiver::onRxDone(rmt_channel_handle_t, const rmt_rx_done_event_data_t* edata,
                               void* ctx) {
  auto* self = static_cast<M5IrTransceiver*>(ctx);
  self->rxCount_ = edata->num_symbols;
  self->rxDone_ = true;
  return false;
}

void M5IrTransceiver::armReceive() {
  rxDone_ = false;
  rxCount_ = 0;
  rmt_receive_config_t cfg = {};
  cfg.signal_range_min_ns = kGlitchNs;
  cfg.signal_range_max_ns = kFrameGapNs;
  rmt_receive(rx_, rxBuf_, sizeof(rxBuf_), &cfg);
}

void M5IrTransceiver::send(const IrFrame& frame) {
  if (frame.empty() || tx_ == nullptr || encoder_ == nullptr) return;
  rmt_symbol_word_t items[IrFrame::kMaxPulses / 2 + 1] = {};
  size_t n = 0;
  for (int i = 0; i < frame.count; i += 2) {
    items[n].level0 = 1;
    items[n].duration0 = frame.pulses[i];
    items[n].level1 = 0;
    items[n].duration1 = i + 1 < frame.count ? frame.pulses[i + 1] : 0;
    ++n;
  }
  rmt_transmit_config_t cfg = {};
  rmt_transmit(tx_, encoder_, items, n * sizeof(items[0]), &cfg);
  rmt_tx_wait_all_done(tx_, -1);
}

void M5IrTransceiver::startLearn() {
  if (learning_ || rx_ == nullptr) return;
  M5.Speaker.end();
  rmt_enable(rx_);
  armReceive();
  learning_ = true;
}

void M5IrTransceiver::stopLearn() {
  if (!learning_) return;
  rmt_disable(rx_);
  M5.Speaker.begin();
  learning_ = false;
}

bool M5IrTransceiver::fetchLearned(IrFrame& out) {
  if (!learning_ || !rxDone_) return false;
  toFrame(rxBuf_, rxCount_, out);
  armReceive();
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
