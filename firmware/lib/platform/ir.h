#pragma once

#include <cstdint>
#include <string>

namespace tama {

// Alternating mark/space durations in microseconds, mark first; replayed
// raw so learned codes stay protocol-agnostic.
struct IrFrame {
  static constexpr int kMaxPulses = 128;

  uint16_t pulses[kMaxPulses] = {0};
  uint8_t count = 0;

  bool empty() const { return count == 0; }
};

struct IrButton {
  std::string label;
  IrFrame frame;
};

class IIrTransceiver {
 public:
  virtual ~IIrTransceiver() = default;
  virtual void begin() = 0;
  virtual void send(const IrFrame& frame) = 0;
  virtual void startLearn() = 0;
  virtual void stopLearn() = 0;
  virtual bool fetchLearned(IrFrame& out) = 0;
};

class IIrStore {
 public:
  virtual ~IIrStore() = default;
  virtual int load(IrButton* out, int max) = 0;
  virtual void save(const IrButton* buttons, int count) = 0;
};

}  // namespace tama
