#include "chat.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp32-hal-log.h>
#include <esp_random.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "b64.h"

namespace tama {

namespace {

constexpr const char* kTag = "chat";
constexpr uint32_t kRate = 16000;
constexpr uint32_t kLimitMs = 15000;
constexpr size_t kStageBytes = 1536;
constexpr size_t kMaxAck = 2048;
constexpr size_t kMaxRows = 64 * 1024;
constexpr size_t kMaxSeen = 4;
constexpr uint32_t kReplyTimeoutMs = 60000;
constexpr uint32_t kSettleMs = 3000;
constexpr uint32_t kResubscribeMs = 2000;

constexpr char kVoiceNote[] = "[Voice note]";
constexpr char kAttachment[] = "\n[file:";

void put32(std::string& out, uint32_t v) {
  for (int i = 0; i < 4; ++i) out.push_back(static_cast<char>(v >> (8 * i)));
}

void put16(std::string& out, uint16_t v) {
  out.push_back(static_cast<char>(v));
  out.push_back(static_cast<char>(v >> 8));
}

std::string wavHeader() {
  std::string h = "RIFF";
  put32(h, 0xFFFFFFFF);
  h += "WAVEfmt ";
  put32(h, 16);
  put16(h, 1);
  put16(h, 1);
  put32(h, kRate);
  put32(h, kRate * 2);
  put16(h, 2);
  put16(h, 16);
  h += "data";
  put32(h, 0xFFFFFFFF);
  return h;
}

std::string requestId() {
  char id[32];
  std::snprintf(id, sizeof(id), "muse-%08lx-%08lx", static_cast<unsigned long>(esp_random()),
                static_cast<unsigned long>(esp_random()));
  return id;
}

}  // namespace

MuseChat::MuseChat(NoiseTransport& link, VoiceChat& voice) : link_(link), voice_(voice) {}

uint32_t MuseChat::limitMs() const { return kLimitMs; }

int64_t MuseChat::open(const char* path, const char* accept, Inbox& inbox, size_t cap) {
  uint32_t generation;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    generation = ++inbox.generation;
    inbox.status = 0;
    inbox.done = false;
    inbox.body.clear();
  }
  const muse::Headers headers = {{"x-request-id", requestId()},
                                 {"x-app-id", "musegadget"},
                                 {"Content-Type", "application/json"},
                                 {"Accept", accept}};
  return link_.open("POST", path, headers, false,
                    [this, &inbox, generation, cap](int status, const uint8_t* data, size_t len,
                                                    bool end) {
                      std::lock_guard<std::mutex> lock(mutex_);
                      if (generation != inbox.generation || inbox.done) return;
                      if (status) inbox.status = status;
                      if (inbox.body.size() + len <= cap)
                        inbox.body.append(reinterpret_cast<const char*>(data), len);
                      inbox.done = end || status < 0;
                    });
}

void MuseChat::reset(Inbox& inbox) {
  std::lock_guard<std::mutex> lock(mutex_);
  ++inbox.generation;
  inbox.status = 0;
  inbox.done = false;
  inbox.body.clear();
}

void MuseChat::subscribe(uint32_t nowMs) {
  if (subscription_ >= 0 || !link_.connected() || static_cast<int32_t>(nowMs - retryAt_) < 0) return;
  lastSeq_ = 0;
  subscription_ = open("/chat/subscribe", "application/x-ndjson", rows_, kMaxRows);
  if (subscription_ < 0 || !link_.send(subscription_, "{}", true)) unsubscribe(nowMs);
}

void MuseChat::unsubscribe(uint32_t nowMs) {
  if (subscription_ >= 0) link_.cancel(subscription_);
  subscription_ = -1;
  reset(rows_);
  retryAt_ = nowMs + kResubscribeMs;
}

void MuseChat::beginRecording() {
  end();
  voice_.reset();
  note_ = open("/chat/stream", "application/json", ack_, kMaxAck);
  if (note_ < 0 || !link_.send(note_, muse::encodeNoteHead(link_.node()), false)) {
    return fail("CAN'T REACH MUSE");
  }
  stage_ = wavHeader();
  turn_ = Turn::Talking;
}

void MuseChat::feed(const int16_t* pcm, size_t samples) {
  const auto* p = reinterpret_cast<const char*>(pcm);
  size_t n = samples * 2;
  while (turn_ == Turn::Talking && n) {
    const size_t take = std::min(kStageBytes - stage_.size(), n);
    stage_.append(p, take);
    p += take;
    n -= take;
    if (stage_.size() == kStageBytes && !flush(false)) fail("CAN'T KEEP UP");
  }
}

bool MuseChat::flush(bool last) {
  std::string chunk = b64::encode(reinterpret_cast<const uint8_t*>(stage_.data()), stage_.size());
  if (last) chunk += muse::encodeNoteTail();
  stage_.clear();
  return link_.send(note_, chunk, last);
}

void MuseChat::finish(uint32_t) {
  if (turn_ != Turn::Talking) {
    if (voice_.error.empty()) fail("CAN'T REACH MUSE");
    return;
  }
  if (subscription_ < 0) return fail("CAN'T SUBSCRIBE TO MUSE");
  if (!flush(true)) return fail("CAN'T KEEP UP");
  endedAt_ = now_;
  turn_ = Turn::Ack;
}

void MuseChat::cancel() {
  end();
  voice_.reset();
}

void MuseChat::loop(uint32_t nowMs) {
  now_ = nowMs;
  if (!link_.connected()) {
    if (subscription_ >= 0) unsubscribe(nowMs);
    if (turn_ == Turn::Ack || turn_ == Turn::Reply) fail("LOST CONNECTION TO MUSE");
    return;
  }
  subscribe(nowMs);

  int status;
  bool closed;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    status = rows_.status;
    closed = rows_.done;
  }
  if (subscription_ >= 0 && (closed || status < 0 || status >= 400)) {
    ESP_LOGW(kTag, "subscription ended (status %d)", status);
    unsubscribe(nowMs);
    if (turn_ == Turn::Ack || turn_ == Turn::Reply) {
      if (status == 403) return fail("MUSE REPLY ACCESS DENIED");
      if (status == 401) return fail("MUSE REPLY AUTH REQUIRED");
      return fail("LOST CONNECTION TO MUSE");
    }
    return;
  }

  drain(nowMs);
  settle(nowMs);
}

void MuseChat::settle(uint32_t nowMs) {
  if (turn_ == Turn::Ack) {
    Inbox ack;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (ack_.done) ack = ack_;
    }
    if (ack.done) {
      if (ack.status != 200) {
        return fail(ack.status < 0 ? "LOST CONNECTION TO MUSE" : "MUSE DIDN'T TAKE IT");
      }
      muse::Ack ids;
      if (!muse::decodeAck(ack.body, ids)) return fail("MUSE DIDN'T TAKE IT");
      noteId_ = ids.message;
      parentId_ = ids.parent;
      turn_ = Turn::Reply;
      if (voice_.reply.empty()) voice_.phase = VoicePhase::Thinking;
    }
  }
  if (turn_ != Turn::Ack && turn_ != Turn::Reply) return;

  const bool replied = !voice_.reply.empty();
  if (replied && nowMs - repliedAt_ > kSettleMs) {
    voice_.reply_done = true;
    end();
  } else if (!replied && nowMs - endedAt_ > kReplyTimeoutMs) {
    fail("NO REPLY FROM MUSE");
  }
}

void MuseChat::drain(uint32_t nowMs) {
  std::string lines;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (rows_.status != 200) return;
    const size_t cut = rows_.body.rfind('\n');
    if (cut == std::string::npos) return;
    lines = rows_.body.substr(0, cut + 1);
    rows_.body.erase(0, cut + 1);
  }
  size_t start = 0;
  while (start < lines.size()) {
    size_t stop = lines.find('\n', start);
    if (stop == std::string::npos) stop = lines.size();
    const std::string line = lines.substr(start, stop - start);
    start = stop + 1;

    muse::Row row;
    if (line.empty() || !muse::decodeRow(line, row)) continue;
    if (row.seq && row.seq <= lastSeq_) continue;
    if (row.seq) lastSeq_ = row.seq;

    std::string id;
    muse::Invoke invoke;
    if (muse::decodeClientInvoke(line, id, invoke)) {
      if (invoke_) invoke_(id, invoke);
      continue;
    }
    if (turn_ == Turn::Ack || turn_ == Turn::Reply) process(row, nowMs);
  }
}

void MuseChat::process(const muse::Row& incoming, uint32_t nowMs) {
  const std::string& event = incoming.event;
  const bool start = event == "delta.message_start";
  const bool append = event == "delta.text_append";
  if ((start || append) && !incoming.message.empty()) {
    if (delta_.message != incoming.message) {
      delta_ = muse::Row{};
      delta_.message = incoming.message;
      delta_.replyTo = incoming.replyTo;
    }
    if (append) delta_.text += incoming.text;
    return;
  }

  const bool user = event == "message.user";
  const bool done = event == "delta.message_done";
  if ((!user && !done && event != "message.assistant") || incoming.message.empty()) return;
  if (!done && !incoming.ready) return;

  muse::Row row = incoming;
  if (row.message == delta_.message) {
    if (row.text.empty()) row.text = delta_.text;
    row.replyTo = delta_.replyTo;
  }
  if (row.text.empty()) {
    ESP_LOGW(kTag, "%s %s has no text", event.c_str(), row.message.c_str());
    return;
  }

  if (user) {
    if (row.message != noteId_ && row.message != parentId_) return;
    std::string heard = row.text.substr(0, row.text.find(kAttachment));
    if (!heard.empty() && heard != kVoiceNote) voice_.transcript = heard;
    return;
  }
  const bool ours = row.replyTo.empty() || noteId_.empty() || row.replyTo == noteId_ ||
                    row.replyTo == parentId_ ||
                    std::find(seen_.begin(), seen_.end(), row.replyTo) != seen_.end();
  if (!ours) {
    ESP_LOGW(kTag, "ignoring %s: replies to %s", row.message.c_str(), row.replyTo.c_str());
    return;
  }
  if (std::find(seen_.begin(), seen_.end(), row.message) != seen_.end()) return;
  if (seen_.size() == kMaxSeen) seen_.erase(seen_.begin());
  seen_.push_back(row.message);

  if (!voice_.reply.empty()) voice_.reply += " ";
  voice_.reply += row.text;
  voice_.phase = VoicePhase::Reply;
  repliedAt_ = nowMs;
}

void MuseChat::fail(const char* why) {
  ESP_LOGW(kTag, "turn failed: %s", why);
  end();
  voice_.error = why;
  voice_.phase = VoicePhase::Reply;
  voice_.reply_done = true;
}

void MuseChat::end() {
  if (note_ >= 0) link_.cancel(note_);
  note_ = -1;
  reset(ack_);
  turn_ = Turn::Idle;
  stage_.clear();
  noteId_.clear();
  parentId_.clear();
  delta_ = muse::Row{};
  seen_.clear();
}

}  // namespace tama

#endif
