#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

#include "model.h"
#include "transport.h"
#include "wifi/control.h"

namespace tama {

class ClaudeSession;
class ICodec;

class IInboundPipeline {
 public:
  virtual ~IInboundPipeline() = default;
  virtual void onConnected(bool connected) { (void)connected; }
  virtual void onInbound(const std::string& topic, const std::string& payload) {
    (void)topic;
    (void)payload;
  }
  virtual void onGesture(const GestureEvent& event) { (void)event; }
  virtual void onMotion(const MotionEvent& event) { (void)event; }
  virtual void tick(uint32_t nowMs) { (void)nowMs; }
};

class TransportProxy : public ITransport {
 public:
  void bind(ITransport* impl) { impl_ = impl; }
  void begin() override { impl_->begin(); }
  void loop() override { impl_->loop(); }
  void publish(const std::string& topic, const std::string& payload, uint8_t qos,
               bool retain) override {
    impl_->publish(topic, payload, qos, retain);
  }
  bool connected() const override { return impl_ != nullptr && impl_->connected(); }
  void onMessage(MessageHandler handler) override { impl_->onMessage(std::move(handler)); }
  void onConnection(ConnectionHandler handler) override { impl_->onConnection(std::move(handler)); }

 private:
  ITransport* impl_ = nullptr;
};

struct SourceChannel {
  IConnection* connection = nullptr;
  IInboundPipeline* pipeline = nullptr;
  bool valid() const { return connection != nullptr && pipeline != nullptr; }
};

struct SourceBinding {
  SourceChannel hub;
  SourceChannel claude;
  ILink* link = nullptr;
  IWifiControl* wifi = nullptr;
  std::function<void(const Page&, PromptOutcome)> resolvePrompt;
};

class Source {
 public:
  void bind(const SourceBinding& binding) {
    hub_ = binding.hub;
    claude_ = binding.claude;
  }

  SourceChannel& hub() { return hub_; }
  SourceChannel& claude() { return claude_; }

 private:
  SourceChannel hub_;
  SourceChannel claude_;
};

std::function<void(const std::string&, PromptOutcome)> makeHubResolver(ITransport& transport,
                                                                         const ICodec& codec,
                                                                         std::string deviceId);
std::function<void(const std::string&, PromptOutcome)> makeClaudeResolver(ClaudeSession& session);

}  // namespace tama
