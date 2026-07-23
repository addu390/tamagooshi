#pragma once

#include "board.gen.h"

#if TAMA_BOARD_HAS_IR

#include <driver/rmt_rx.h>
#include <driver/rmt_tx.h>

#include "ir.h"

namespace tama {

// The StickS3 receiver only decodes via RMT and shares the audio amp rail,
// so the speaker is parked while learning.
class M5IrTransceiver : public IIrTransceiver {
 public:
  M5IrTransceiver(int txPin, int rxPin) : txPin_(txPin), rxPin_(rxPin) {}

  void begin() override;
  void send(const IrFrame& frame) override;
  void startLearn() override;
  void stopLearn() override;
  bool fetchLearned(IrFrame& out) override;

 private:
  static bool onRxDone(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t* edata,
                       void* ctx);
  void armReceive();

  static constexpr size_t kRxBufSymbols = 128;

  int txPin_;
  int rxPin_;
  rmt_channel_handle_t tx_ = nullptr;
  rmt_channel_handle_t rx_ = nullptr;
  rmt_encoder_handle_t encoder_ = nullptr;
  rmt_symbol_word_t rxBuf_[kRxBufSymbols] = {};
  volatile size_t rxCount_ = 0;
  volatile bool rxDone_ = false;
  bool learning_ = false;
};

class NvsIrStore : public IIrStore {
 public:
  int load(IrButton* out, int max) override;
  void save(const IrButton* buttons, int count) override;
};

}  // namespace tama

#endif  // TAMA_BOARD_HAS_IR
