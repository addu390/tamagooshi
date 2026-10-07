#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "bearer.h"
#include "muse.h"
#include "transport.h"

namespace tama {

namespace muse {

struct Target {
  std::string host;
  std::string vm;
  std::string token;
};

enum class Resolution { Ok, Failed, Revoked };
enum class Fault { None, Unpaired, Revoked };

using Resolver = std::function<Resolution(Target&)>;
using Headers = std::vector<std::pair<std::string, std::string>>;
using StreamHandler = std::function<void(int status, const uint8_t* data, size_t len, bool end)>;

}  // namespace muse

class NoiseTransport : public ITransport {
 public:
  NoiseTransport(IBearer& bearer, muse::Registration registration, muse::Resolver resolve);
  ~NoiseTransport() override;

  void begin() override {}
  void loop() override;
  void publish(const std::string& topic, const std::string& payload) override;
  bool connected() const override { return registered_; }
  void onMessage(MessageHandler handler) override { messageHandler_ = std::move(handler); }
  void onConnection(ConnectionHandler handler) override {
    connectionHandler_ = std::move(handler);
  }

  void start(const std::string& ssid);
  void stop();
  muse::Fault fault();
  std::string agent() const;
  const std::string& node() const { return registration_.node; }

  int64_t open(const char* verb, const char* path, const muse::Headers& headers, bool end,
               muse::StreamHandler handler);
  bool send(int64_t stream, const std::string& data, bool end);
  void cancel(int64_t stream);

 private:
  class Session;
  enum class Outcome { Closed, Rejected, Unpaired };

  struct Op {
    enum class Kind { Open, Body, Cancel } kind;
    int64_t stream = 0;
    std::string verb;
    std::string path;
    muse::Headers headers;
    std::string data;
    bool end = false;
    muse::StreamHandler handler;
  };

  static void entry(void* self);
  void run();
  Outcome connect(const muse::Target& target);
  void sleep(uint32_t ms);
  void raise(muse::Fault fault);
  void setRegistered(bool on);

  IBearer& bearer_;
  muse::Registration registration_;
  muse::Resolver resolve_;

  std::atomic<bool> running_{false};
  std::atomic<bool> registered_{false};
  std::atomic<int64_t> nextStream_;
  std::atomic<bool> alive_{false};

  mutable std::mutex mutex_;
  std::deque<std::pair<std::string, std::string>> inbound_;
  std::deque<bool> transitions_;
  std::deque<std::string> results_;
  std::deque<Op> ops_;
  size_t queuedBytes_ = 0;
  muse::Fault fault_ = muse::Fault::None;
  std::string agent_;

  MessageHandler messageHandler_;
  ConnectionHandler connectionHandler_;
};

}  // namespace tama

#endif
