#include "channels.h"

#include "codec.h"
#include "topics.h"

namespace tama {

std::function<void(const std::string&, PromptOutcome)> makeHubResolver(ITransport& transport,
                                                                       const ICodec& codec,
                                                                       std::string deviceId) {
  return [&transport, &codec, deviceId](const std::string& pageId, PromptOutcome outcome) {
    if (outcome == PromptOutcome::Ack)
      transport.publish(topics::acks(deviceId), codec.encodeAck(pageId), 1, false);
  };
}

}  // namespace tama
