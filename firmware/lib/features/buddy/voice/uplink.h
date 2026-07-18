#pragma once

#include <cstdint>
#include <string>

#include "adpcm.h"
#include "model.h"
#include "transport.h"

namespace tama {

class VoiceUplink : public IVoiceUplink {
 public:
  VoiceUplink(ILineSink& sink, DeviceState& state, size_t capacityBytes);

  bool ready() const override;
  void beginRecording() override;
  void feed(const int16_t* pcm, size_t samples) override;
  void finish(uint32_t elapsedMs, const std::string& agent) override;
  void cancel() override;
  bool sending() const override { return sending_; }
  size_t capacityBytes() const override { return capacity_; }
  size_t bufferedBytes() const override { return buffer_.size(); }

  void pump(uint32_t nowMs);

 private:
  void reset();

  ILineSink& sink_;
  DeviceState& state_;
  size_t capacity_;

  adpcm::State encoder_;
  std::string buffer_;
  std::string agent_;
  size_t cursor_ = 0;
  int seq_ = 0;
  uint32_t elapsed_ms_ = 0;
  uint32_t next_send_ms_ = 0;
  bool sending_ = false;
};

}  // namespace tama
