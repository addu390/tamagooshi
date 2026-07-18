#include "source.h"

#include "buddy/claude/session.h"
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

std::function<void(const std::string&, PromptOutcome)> makeClaudeResolver(ClaudeSession& session) {
  return [&session](const std::string& id, PromptOutcome outcome) {
    if (outcome == PromptOutcome::Allow)
      session.decide(id, true);
    else if (outcome == PromptOutcome::Deny)
      session.decide(id, false);
  };
}

}  // namespace tama
