#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "model.h"
#include "muse.h"
#include "transport/noise.h"

namespace tama {

// Voice turns plus the connection-long chat subscription. Muse sends commands
// for turns this device started as client invokes on that subscription.
class MuseChat : public IVoiceUplink {
 public:
  using InvokeHandler = std::function<void(const std::string& id, const muse::Invoke& invoke)>;

  MuseChat(NoiseTransport& link, VoiceChat& voice);

  bool ready() const override { return link_.connected(); }
  void beginRecording() override;
  void feed(const int16_t* pcm, size_t samples) override;
  void finish(uint32_t elapsedMs) override;
  void cancel() override;
  bool sending() const override { return turn_ == Turn::Ack || turn_ == Turn::Reply; }
  uint32_t limitMs() const override;

  void onInvoke(InvokeHandler handler) { invoke_ = std::move(handler); }
  void loop(uint32_t nowMs);

 private:
  enum class Turn { Idle, Talking, Ack, Reply };

  struct Inbox {
    uint32_t generation = 0;
    int status = 0;
    bool done = false;
    std::string body;
  };

  int64_t open(const char* path, const char* accept, Inbox& inbox, size_t cap);
  void reset(Inbox& inbox);
  void subscribe(uint32_t nowMs);
  void unsubscribe(uint32_t nowMs);
  bool flush(bool last);
  void drain(uint32_t nowMs);
  void process(const muse::Row& row, uint32_t nowMs);
  void settle(uint32_t nowMs);
  void fail(const char* why);
  void end();

  NoiseTransport& link_;
  VoiceChat& voice_;
  InvokeHandler invoke_;

  int64_t subscription_ = -1;
  uint32_t retryAt_ = 0;
  uint64_t lastSeq_ = 0;

  Turn turn_ = Turn::Idle;
  int64_t note_ = -1;
  std::string stage_;
  std::string noteId_;
  std::string parentId_;
  muse::Row delta_;
  std::vector<std::string> seen_;
  uint32_t endedAt_ = 0;
  uint32_t repliedAt_ = 0;
  uint32_t now_ = 0;

  std::mutex mutex_;
  Inbox ack_;
  Inbox rows_;
};

}  // namespace tama

#endif
