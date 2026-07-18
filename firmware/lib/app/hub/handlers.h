#pragma once

#include "codec.h"
#include "expression.h"
#include "model.h"
#include "router.h"
#include "system.h"

namespace tama {

class StateHandler : public IMessageHandler {
 public:
  StateHandler(DeviceState& state, const ICodec& codec) : state_(state), codec_(codec) {}

 protected:
  DeviceState& state_;
  const ICodec& codec_;
};

class BrandingHandler : public StateHandler {
 public:
  using StateHandler::StateHandler;
  void handle(const Envelope& env) override;
};

class MetricHandler : public StateHandler {
 public:
  using StateHandler::StateHandler;
  void handle(const Envelope& env) override;
};

class MoodHandler : public StateHandler {
 public:
  using StateHandler::StateHandler;
  void handle(const Envelope& env) override;
};

class ConfigHandler : public StateHandler {
 public:
  using StateHandler::StateHandler;
  void handle(const Envelope& env) override;
};

class TimeHandler : public IMessageHandler {
 public:
  TimeHandler(DeviceState& state, ISystemControl& system, const ICodec& codec)
      : state_(state), system_(system), codec_(codec) {}
  void handle(const Envelope& env) override;

 private:
  DeviceState& state_;
  ISystemControl& system_;
  const ICodec& codec_;
};

class PageHandler : public IMessageHandler {
 public:
  PageHandler(DeviceState& state, const ICodec& codec) : state_(state), codec_(codec) {}

 protected:
  DeviceState& state_;
  const ICodec& codec_;
};

class PageRaiseHandler : public PageHandler {
 public:
  using PageHandler::PageHandler;
  void handle(const Envelope& env) override;
};

class PageClearHandler : public PageHandler {
 public:
  using PageHandler::PageHandler;
  void handle(const Envelope& env) override;
};

class ExpressionHandler : public IMessageHandler {
 public:
  ExpressionHandler(IExpressionSink& sink, const ICodec& codec) : sink_(sink), codec_(codec) {}
  void handle(const Envelope& env) override;

 private:
  IExpressionSink& sink_;
  const ICodec& codec_;
};

class VoiceHandlerBase : public IMessageHandler {
 public:
  VoiceHandlerBase(DeviceState& state, IExpressionSink& sink) : state_(state), sink_(sink) {}

 protected:
  DeviceState& state_;
  IExpressionSink& sink_;
};

class VoiceStartHandler : public VoiceHandlerBase {
 public:
  using VoiceHandlerBase::VoiceHandlerBase;
  void handle(const Envelope& env) override;
};

class VoiceStopHandler : public VoiceHandlerBase {
 public:
  using VoiceHandlerBase::VoiceHandlerBase;
  void handle(const Envelope& env) override;
};

class CommandHandler : public IMessageHandler {
 public:
  CommandHandler(ISystemControl& system, IExpressionSink& sink, const ICodec& codec)
      : system_(system), sink_(sink), codec_(codec) {}
  void handle(const Envelope& env) override;

 private:
  ISystemControl& system_;
  IExpressionSink& sink_;
  const ICodec& codec_;
};

struct HandlerSet {
  HandlerSet(DeviceState& state, const ICodec& codec, IExpressionSink& sink,
             ISystemControl& system)
      : branding(state, codec),
        metric(state, codec),
        mood(state, codec),
        config(state, codec),
        time(state, system, codec),
        pageRaise(state, codec),
        pageClear(state, codec),
        expression(sink, codec),
        voiceStart(state, sink),
        voiceStop(state, sink),
        command(system, sink, codec) {}

  void bind(MessageRouter& router);

  BrandingHandler branding;
  MetricHandler metric;
  MoodHandler mood;
  ConfigHandler config;
  TimeHandler time;
  PageRaiseHandler pageRaise;
  PageClearHandler pageClear;
  ExpressionHandler expression;
  VoiceStartHandler voiceStart;
  VoiceStopHandler voiceStop;
  CommandHandler command;
};

}  // namespace tama
