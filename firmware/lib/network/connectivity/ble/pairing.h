#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <cstdint>
#include <string>
#include <utility>

#include "muse.h"

namespace tama {

class Pairing {
 public:
  enum class Phase { Idle, Finishing, Confirming, Ready };

  explicit Pairing(muse::Device device) : device_(std::move(device)) {}
  ~Pairing() { reset(); }

  Pairing(const Pairing&) = delete;
  Pairing& operator=(const Pairing&) = delete;

  bool hello(const muse::Hello& hello, muse::Ready& out);
  bool open(const muse::Sealed& sealed, std::string& plain);
  bool seal(const std::string& plain, muse::Sealed& out);
  bool finish();
  bool confirm();
  bool expired() const;
  void reset();

  Phase phase() const { return phase_; }
  bool keyed() const { return phase_ != Phase::Idle; }
  const muse::Device& device() const { return device_; }

 private:
  void advance(Phase phase, int64_t timeoutUs);

  muse::Device device_;
  Phase phase_ = Phase::Idle;
  int64_t deadline_ = 0;
  uint8_t rx_[32] = {};
  uint8_t tx_[32] = {};
  std::string session_;
  uint64_t rxCounter_ = 0;
  uint64_t txCounter_ = 0;
};

}  // namespace tama

#endif
