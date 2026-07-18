#pragma once

#include <string>
#include <vector>

#include "model.h"

namespace tama {

class VoiceController {
 public:
  explicit VoiceController(DeviceState& state);

  void applyAgents(const std::vector<std::string>& agents, const std::string& preferred);
  void applyTranscript(const std::string& id, const std::string& text, const std::string& agent);
  void applyReply(const std::string& text, bool done);
  void onDecision(const std::string& id, bool send);
  void onConnected(bool connected);

 private:
  DeviceState& state_;
};

}  // namespace tama
