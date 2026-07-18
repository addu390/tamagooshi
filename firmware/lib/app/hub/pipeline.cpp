#include "pipeline.h"

#include <utility>

#include "topics.h"

namespace tama {

HubPipeline::HubPipeline(ITransport& transport, const ICodec& codec, MessageRouter& router,
                         IBoardProfile& board, std::string deviceId, std::string fwVersion)
    : transport_(transport),
      codec_(codec),
      router_(router),
      board_(board),
      deviceId_(std::move(deviceId)),
      fwVersion_(std::move(fwVersion)) {}

void HubPipeline::onConnected(bool connected) {
  if (!connected) return;
  transport_.publish(topics::hello(deviceId_), codec_.encodeHello(board_.capabilities(), fwVersion_),
                     1, true);
  transport_.publish(topics::status(deviceId_), codec_.encodeStatus(true, fwVersion_, ""), 1, true);
}

void HubPipeline::onInbound(const std::string&, const std::string& payload) {
  Envelope env;
  if (!codec_.decodeEnvelope(payload, env)) return;
  router_.dispatch(env);
}

void HubPipeline::onGesture(const GestureEvent& event) {
  transport_.publish(topics::input(deviceId_), codec_.encodeGesture(event), 1, false);
}

void HubPipeline::onMotion(const MotionEvent& event) {
  transport_.publish(topics::sensor(deviceId_), codec_.encodeMotion(event), 1, false);
}

}  // namespace tama
