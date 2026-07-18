#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tama {

enum class VoicePhase { Idle, Recording, Sending, Confirming, Thinking, Reply };

struct AgentRoster {
  std::vector<std::string> options;
  int selected = 0;

  bool multiple() const { return options.size() > 1; }

  const std::string& current() const {
    static const std::string kNone;
    if (options.empty()) return kNone;
    return options[static_cast<size_t>(selected) % options.size()];
  }

  void cycle() {
    if (multiple()) selected = (selected + 1) % static_cast<int>(options.size());
  }

  void set(std::vector<std::string> agents, const std::string& preferred) {
    options = std::move(agents);
    selected = 0;
    for (size_t i = 0; i < options.size(); ++i) {
      if (options[i] == preferred) selected = static_cast<int>(i);
    }
  }

  void clear() {
    options.clear();
    selected = 0;
  }
};

struct VoiceChat {
  VoicePhase phase = VoicePhase::Idle;
  std::string transcript;
  std::string transcript_id;
  std::string agent;
  std::string reply;
  bool reply_done = false;

  void reset() {
    phase = VoicePhase::Idle;
    transcript.clear();
    transcript_id.clear();
    agent.clear();
    reply.clear();
    reply_done = false;
  }
};

class IVoiceUplink {
 public:
  virtual ~IVoiceUplink() = default;
  virtual bool ready() const = 0;
  virtual void beginRecording() = 0;
  virtual void feed(const int16_t* pcm, size_t samples) = 0;
  virtual void finish(uint32_t elapsedMs, const std::string& agent) = 0;
  virtual void cancel() = 0;
  virtual bool sending() const = 0;
  virtual size_t capacityBytes() const = 0;
  virtual size_t bufferedBytes() const = 0;
};

}  // namespace tama
