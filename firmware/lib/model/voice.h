#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tama {

enum class VoicePhase { Idle, Recording, Sending, Thinking, Reply };

struct VoiceChat {
  VoicePhase phase = VoicePhase::Idle;
  std::string transcript;
  std::string reply;
  std::string error;
  bool reply_done = false;

  void reset() {
    phase = VoicePhase::Idle;
    transcript.clear();
    reply.clear();
    error.clear();
    reply_done = false;
  }
};

class IVoiceUplink {
 public:
  virtual ~IVoiceUplink() = default;
  virtual bool ready() const = 0;
  virtual void beginRecording() = 0;
  virtual void feed(const int16_t* pcm, size_t samples) = 0;
  virtual void finish(uint32_t elapsedMs) = 0;
  virtual void cancel() = 0;
  virtual bool sending() const = 0;
  virtual uint32_t limitMs() const = 0;
};

}  // namespace tama
