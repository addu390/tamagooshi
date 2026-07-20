#pragma once

#include "board.gen.h"

#if TAMA_BOARD_HAS_IR

#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>

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
  int txPin_;
  int rxPin_;
  RingbufHandle_t rx_ = nullptr;
  bool learning_ = false;
};

class NvsIrStore : public IIrStore {
 public:
  int load(IrButton* out, int max) override;
  void save(const IrButton* buttons, int count) override;
};

}  // namespace tama

#endif  // TAMA_BOARD_HAS_IR
