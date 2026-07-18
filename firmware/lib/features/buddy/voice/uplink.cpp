#include "uplink.h"

#include <ArduinoJson.h>

#include <algorithm>

#include "b64.h"

namespace tama {

namespace {
constexpr size_t kLineRawBytes = 512;
constexpr uint32_t kSendIntervalMs = 30;
}  // namespace

VoiceUplink::VoiceUplink(ILineSink& sink, DeviceState& state, size_t capacityBytes)
    : sink_(sink), state_(state), capacity_(capacityBytes) {}

bool VoiceUplink::ready() const { return state_.agent_connected; }

void VoiceUplink::beginRecording() {
  reset();
  buffer_.reserve(capacity_);
}

void VoiceUplink::feed(const int16_t* pcm, size_t samples) {
  if (sending_ || buffer_.size() >= capacity_) return;
  uint8_t encoded[256];
  while (samples > 0) {
    const size_t take = std::min(samples, sizeof(encoded) * 2);
    const size_t n = adpcm::encode(encoder_, pcm, take, encoded);
    const size_t room = capacity_ - buffer_.size();
    buffer_.append(reinterpret_cast<const char*>(encoded), std::min(n, room));
    if (n >= room) return;
    pcm += take;
    samples -= take;
  }
}

void VoiceUplink::finish(uint32_t elapsedMs, const std::string& agent) {
  if (buffer_.empty()) {
    reset();
    return;
  }
  agent_ = agent;
  elapsed_ms_ = elapsedMs;
  cursor_ = 0;
  seq_ = 0;
  next_send_ms_ = 0;
  sending_ = true;
}

void VoiceUplink::cancel() { reset(); }

void VoiceUplink::pump(uint32_t nowMs) {
  if (!sending_ || nowMs < next_send_ms_) return;
  next_send_ms_ = nowMs + kSendIntervalMs;

  if (cursor_ < buffer_.size()) {
    const size_t n = std::min(kLineRawBytes, buffer_.size() - cursor_);
    JsonDocument doc;
    doc["cmd"] = "voice";
    doc["seq"] = seq_++;
    doc["data"] = b64::encode(reinterpret_cast<const uint8_t*>(buffer_.data() + cursor_), n);
    cursor_ += n;
    std::string line;
    serializeJson(doc, line);
    sink_.send(line);
    return;
  }

  JsonDocument doc;
  doc["cmd"] = "voice_end";
  doc["ms"] = elapsed_ms_;
  if (!agent_.empty()) doc["agent"] = agent_;
  std::string line;
  serializeJson(doc, line);
  sink_.send(line);
  reset();
}

void VoiceUplink::reset() {
  encoder_ = adpcm::State{};
  buffer_.clear();
  buffer_.shrink_to_fit();
  agent_.clear();
  cursor_ = 0;
  seq_ = 0;
  elapsed_ms_ = 0;
  next_send_ms_ = 0;
  sending_ = false;
}

}  // namespace tama
