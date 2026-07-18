#pragma once

#include <string>

#include "board.h"
#include "codec.h"
#include "router.h"
#include "source.h"
#include "transport.h"

namespace tama {

class HubPipeline : public IInboundPipeline {
 public:
  HubPipeline(ITransport& transport, const ICodec& codec, MessageRouter& router, IBoardProfile& board,
              std::string deviceId, std::string fwVersion);

  void onConnected(bool connected) override;
  void onInbound(const std::string& topic, const std::string& payload) override;
  void onGesture(const GestureEvent& event) override;
  void onMotion(const MotionEvent& event) override;

 private:
  ITransport& transport_;
  const ICodec& codec_;
  MessageRouter& router_;
  IBoardProfile& board_;
  std::string deviceId_;
  std::string fwVersion_;
};

}  // namespace tama
