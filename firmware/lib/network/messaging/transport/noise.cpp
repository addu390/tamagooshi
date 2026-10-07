#include "transport/noise.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp_heap_caps.h>
#include <esp32-hal-log.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <optional>

#include "socket.h"
#include "noise/ClientSession.h"
#include "noise/MbedtlsCryptoBackend.h"

namespace tama {

namespace noise = musegadgets::noise::core;

namespace {

constexpr const char* kTag = "noise";

constexpr int kPort = 443;
constexpr int64_t kControlStream = 1;
constexpr int64_t kIdentityStream = 3;
constexpr int64_t kFirstRequestStream = 16;
constexpr size_t kMaxRequestStreams = 3;
constexpr size_t kMaxHeaders = 16;

constexpr size_t kWsBuffer = noise::ClientSession::kMaxOutboundWebSocketPayloadSize;
constexpr size_t kScratch = 12288;
constexpr size_t kChunk = 8192;
constexpr size_t kMaxControlMessage = 256 * 1024;
constexpr size_t kMaxIdentity = 2048;
constexpr size_t kMaxQueuedBytes = 16 * 1024;
constexpr uint32_t kStackSize = 12288;

constexpr int64_t kKeepaliveUs = 60LL * 1000000;
constexpr int64_t kHeartbeatUs = 24LL * 3600 * 1000000;
constexpr int64_t kSilenceUs = 3 * kKeepaliveUs;
constexpr uint32_t kShortSessionMs = 30000;
constexpr uint32_t kMinBackoffMs = 1000;
constexpr uint32_t kMaxBackoffMs = 15000;
constexpr uint32_t kAuthBackoffMs = 60000;
constexpr uint8_t kShortFailuresBeforeRefresh = 3;
constexpr uint32_t kOfflinePollMs = 500;

uint32_t nowMs() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

struct Buffer {
  explicit Buffer(size_t size)
      : data(static_cast<uint8_t*>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT))),
        size(size) {}
  ~Buffer() { heap_caps_free(data); }
  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  noise::ByteSpan span() const { return noise::ByteSpan(data, size); }

  uint8_t* data;
  size_t size;
};

std::string requestId() {
  char id[24];
  std::snprintf(id, sizeof(id), "reg-%08lx%08lx", static_cast<unsigned long>(esp_random()),
                static_cast<unsigned long>(esp_random()));
  return id;
}

std::string escape(const std::string& value) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  for (unsigned char c : value) {
    if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += static_cast<char>(c);
    } else {
      out += '%';
      out += kHex[c >> 4];
      out += kHex[c & 0xF];
    }
  }
  return out;
}

}  // namespace

class NoiseTransport::Session {
 public:
  Session(NoiseTransport& owner, Socket& socket)
      : owner_(owner),
        socket_(socket),
        session_(crypto_),
        ws_(kWsBuffer),
        rx_(kWsBuffer),
        svc_(kScratch),
        env_(kScratch),
        frame_(kScratch),
        response_(kScratch) {}

  ~Session() {
    for (auto& stream : streams_) stream.second(-1, nullptr, 0, true);
  }

  Outcome run() {
    if (!ws_.data || !rx_.data || !svc_.data || !env_.data || !frame_.data || !response_.data)
      return Outcome::Closed;
    if (!handshake() || !openControl()) {
      ESP_LOGW(kTag, "noise handshake failed");
      return Outcome::Rejected;
    }
    registerId_ = requestId();
    if (!sendControl(muse::encodeRegister(registerId_, owner_.registration_))) return Outcome::Closed;

    int64_t lastActivity = esp_timer_get_time();
    int64_t lastHeard = lastActivity;
    while (owner_.running_) {
      bool busy = false;
      const int n = socket_.recv(rx_.data, rx_.size, false);
      if (n == Socket::kClosed) return Outcome::Closed;
      if (n >= 0 || n == Socket::kControl) lastActivity = lastHeard = esp_timer_get_time();
      if (n >= 0) {
        busy = true;
        const auto result = inbound(static_cast<size_t>(n));
        if (result) return *result;
      }

      if (!transmit(busy)) return Outcome::Closed;
      if (!session_.isEstablished()) return Outcome::Closed;

      const int64_t now = esp_timer_get_time();
      if (now - lastHeard >= kSilenceUs) {
        ESP_LOGW(kTag, "server silent for %llds", static_cast<long long>((now - lastHeard) / 1000000));
        return Outcome::Closed;
      }
      if (busy) {
        lastActivity = now;
      } else if (now - lastActivity >= kKeepaliveUs) {
        if (!socket_.ping()) return Outcome::Closed;
        lastActivity = now;
      } else {
        socket_.idle(streams_.empty() ? 50 : 10);
      }
    }
    return Outcome::Closed;
  }

 private:
  using Result = std::optional<Outcome>;

  bool handshake() {
    const auto first = session_.WriteHandshakeMessage1(ws_.span());
    if (!first.ok() || !socket_.send(ws_.data, first.size())) return false;
    const int n = socket_.recv(ws_.data, ws_.size, true);
    if (n <= 0) return false;
    uint8_t extra[256];
    size_t extraLen = 0;
    if (!session_
             .ReadHandshakeMessage2(noise::ConstByteSpan(ws_.data, static_cast<size_t>(n)),
                                    noise::ByteSpan(extra, sizeof(extra)), extraLen)
             .ok())
      return false;
    const auto third = session_.WriteHandshakeMessage3(noise::ConstByteSpan(), ws_.span());
    if (!third.ok() || !socket_.send(ws_.data, third.size())) return false;
    return session_.isEstablished();
  }

  bool openControl() {
    noise::ApplicationRequestView request;
    request.verb = "POST";
    request.path = "/link-control";
    request.end_body = false;
    return start(session_.StartOutboundApplicationRequest(
        noise::ServiceType::Daemon, kControlStream, request, svc_.span(), env_.span()));
  }

  bool start(const noise::StatusWithSize& status) { return status.ok() && flush(); }

  bool flush() {
    while (session_.HasOutboundWebSocketPayload()) {
      const auto payload = session_.WriteNextOutboundWebSocketPayload(ws_.span());
      if (!payload.ok() || !socket_.send(ws_.data, payload.size())) return false;
    }
    return true;
  }

  bool body(int64_t stream, const uint8_t* data, size_t len, bool end) {
    noise::BodyChunkView chunk;
    chunk.data = noise::ConstByteSpan(data, len);
    chunk.end_body = end;
    return start(session_.StartOutboundBodyChunk(noise::ServiceType::Daemon, stream, chunk,
                                                 svc_.span(), env_.span()));
  }

  bool sendControl(const std::string& message) {
    const uint32_t size = static_cast<uint32_t>(message.size());
    std::string framed(4, '\0');
    for (int i = 0; i < 4; ++i) framed[i] = static_cast<char>(size >> (8 * i));
    framed += message;
    const auto* data = reinterpret_cast<const uint8_t*>(framed.data());
    for (size_t off = 0; off < framed.size(); off += kChunk) {
      const size_t len = std::min(kChunk, framed.size() - off);
      if (!body(kControlStream, data + off, len, false)) return false;
    }
    return true;
  }

  Result inbound(size_t len) {
    noise::HeaderView headers[kMaxHeaders];
    const auto in = session_.ProcessInboundWebSocketPayload(
        noise::ConstByteSpan(rx_.data, len), frame_.span(), response_.span(),
        noise::Span<noise::HeaderView>(headers, kMaxHeaders));
    if (!in.ok()) return closed();
    if (in.frame_status != noise::InboundFrameStatus::Complete) return std::nullopt;

    const auto& frame = in.frame;
    if (frame.stream_id == kControlStream) return control(frame);
    if (frame.stream_id == kIdentityStream) {
      identity(frame);
      return std::nullopt;
    }
    stream(frame);
    return std::nullopt;
  }

  Result control(const noise::DecodedServiceFrame& frame) {
    switch (frame.kind) {
      case noise::ServiceFrameKind::Reset:
        ESP_LOGW(kTag, "control stream reset");
        return closed();
      case noise::ServiceFrameKind::Response:
        if (frame.response.status != 200) {
          ESP_LOGW(kTag, "control stream refused: HTTP %d", frame.response.status);
          return closed();
        }
        return messages(frame.response.body, frame.response.end_body);
      case noise::ServiceFrameKind::BodyChunk:
        return messages(frame.body_chunk.data, frame.body_chunk.end_body);
      default:
        return std::nullopt;
    }
  }

  Result messages(noise::ConstByteSpan data, bool ended) {
    control_.append(reinterpret_cast<const char*>(data.data()), data.size());
    while (control_.size() >= 4) {
      const auto* p = reinterpret_cast<const uint8_t*>(control_.data());
      const size_t len = p[0] | (p[1] << 8) | (p[2] << 16) | (size_t(p[3]) << 24);
      if (len > kMaxControlMessage) return closed();
      if (control_.size() < 4 + len) break;
      const std::string message = control_.substr(4, len);
      control_.erase(0, 4 + len);
      if (message.empty()) continue;

      const muse::Control c = muse::decodeControl(message, registerId_);
      switch (c.kind) {
        case muse::ControlKind::Registered:
          heartbeats_ = c.heartbeats;
          owner_.setRegistered(true);
          ESP_LOGI(kTag, "registered (heartbeats %s)", heartbeats_ ? "on" : "off");
          if (!requestIdentity()) return closed();
          break;
        case muse::ControlKind::Rejected:
          ESP_LOGE(kTag, "registration rejected");
          return closed();
        case muse::ControlKind::Unpaired:
          return Outcome::Unpaired;
        case muse::ControlKind::Invoke: {
          std::lock_guard<std::mutex> lock(owner_.mutex_);
          owner_.inbound_.emplace_back(c.id, message);
          break;
        }
        case muse::ControlKind::Ignored:
          break;
      }
    }
    if (ended) {
      ESP_LOGW(kTag, "control stream ended by server");
      return closed();
    }
    return std::nullopt;
  }

  bool requestIdentity() {
    noise::ApplicationRequestView request;
    request.verb = "GET";
    request.path = "/identity";
    request.end_body = true;
    return start(session_.StartOutboundApplicationRequest(
        noise::ServiceType::Daemon, kIdentityStream, request, svc_.span(), env_.span()));
  }

  void identity(const noise::DecodedServiceFrame& frame) {
    noise::ConstByteSpan data;
    bool end = false;
    if (frame.kind == noise::ServiceFrameKind::Response && frame.response.status == 200) {
      data = frame.response.body;
      end = frame.response.end_body;
    } else if (frame.kind == noise::ServiceFrameKind::BodyChunk) {
      data = frame.body_chunk.data;
      end = frame.body_chunk.end_body;
    } else {
      identity_.clear();
      return;
    }
    if (identity_.size() + data.size() > kMaxIdentity) {
      identity_.clear();
      return;
    }
    identity_.append(reinterpret_cast<const char*>(data.data()), data.size());
    if (!end) return;
    const std::string name = muse::decodeIdentity(identity_);
    identity_.clear();
    if (name.empty()) return;
    std::lock_guard<std::mutex> lock(owner_.mutex_);
    owner_.agent_ = name;
  }

  void stream(const noise::DecodedServiceFrame& frame) {
    const auto it = streams_.find(frame.stream_id);
    if (it == streams_.end()) return;
    const muse::StreamHandler handler = it->second;
    switch (frame.kind) {
      case noise::ServiceFrameKind::Response:
        handler(frame.response.status, frame.response.body.data(), frame.response.body.size(),
                frame.response.end_body);
        if (frame.response.end_body) streams_.erase(frame.stream_id);
        break;
      case noise::ServiceFrameKind::BodyChunk:
        handler(0, frame.body_chunk.data.data(), frame.body_chunk.data.size(),
                frame.body_chunk.end_body);
        if (frame.body_chunk.end_body) streams_.erase(frame.stream_id);
        break;
      case noise::ServiceFrameKind::Reset:
        handler(-1, nullptr, 0, true);
        streams_.erase(frame.stream_id);
        break;
      default:
        break;
    }
  }

  bool transmit(bool& busy) {
    std::deque<std::string> results;
    std::deque<Op> ops;
    {
      std::lock_guard<std::mutex> lock(owner_.mutex_);
      if (owner_.registered_) results.swap(owner_.results_);
      ops.swap(owner_.ops_);
      owner_.queuedBytes_ = 0;
    }
    busy = busy || !results.empty() || !ops.empty();

    for (const std::string& result : results) {
      if (!sendControl(result)) return false;
    }
    for (Op& op : ops) {
      if (!apply(op)) return false;
    }

    const int64_t now = esp_timer_get_time();
    if (heartbeats_ && (lastHeartbeat_ == 0 || now - lastHeartbeat_ >= kHeartbeatUs)) {
      if (!sendControl(muse::encodeHeartbeat())) return false;
      lastHeartbeat_ = now;
    }
    return true;
  }

  bool apply(Op& op) {
    switch (op.kind) {
      case Op::Kind::Open: {
        noise::HeaderView headers[kMaxHeaders];
        const size_t count = std::min(op.headers.size(), kMaxHeaders);
        for (size_t i = 0; i < count; ++i) headers[i] = {op.headers[i].first, op.headers[i].second};
        noise::ApplicationRequestView request;
        request.verb = op.verb;
        request.path = op.path;
        request.headers = noise::Span<const noise::HeaderView>(headers, count);
        request.end_body = op.end;
        if (streams_.size() >= kMaxRequestStreams) {
          op.handler(-1, nullptr, 0, true);
          return true;
        }
        if (!start(session_.StartOutboundApplicationRequest(noise::ServiceType::Daemon, op.stream,
                                                            request, svc_.span(), env_.span())))
          return false;
        streams_[op.stream] = std::move(op.handler);
        return true;
      }
      case Op::Kind::Body: {
        if (!streams_.count(op.stream)) return true;
        const auto* data = reinterpret_cast<const uint8_t*>(op.data.data());
        size_t off = 0;
        do {
          const size_t len = std::min(kChunk, op.data.size() - off);
          const bool last = off + len == op.data.size();
          if (!body(op.stream, data + off, len, op.end && last)) return false;
          off += len;
        } while (off < op.data.size());
        return true;
      }
      case Op::Kind::Cancel: {
        const auto it = streams_.find(op.stream);
        if (it == streams_.end()) return true;
        streams_.erase(it);
        noise::ResetView reset;
        reset.code = noise::ResetCode::Cancelled;
        reset.reason = "cancelled";
        return start(session_.StartOutboundReset(noise::ServiceType::Daemon, op.stream, reset,
                                                 svc_.span(), env_.span()));
      }
    }
    return true;
  }

  static Result closed() { return Outcome::Closed; }

  NoiseTransport& owner_;
  Socket& socket_;
  noise::MbedtlsCryptoBackend crypto_;
  noise::ClientSession session_;
  Buffer ws_;
  Buffer rx_;
  Buffer svc_;
  Buffer env_;
  Buffer frame_;
  Buffer response_;
  std::string registerId_;
  std::string control_;
  std::string identity_;
  std::map<int64_t, muse::StreamHandler> streams_;
  bool heartbeats_ = false;
  int64_t lastHeartbeat_ = 0;
};

NoiseTransport::NoiseTransport(IBearer& bearer, muse::Registration registration,
                             muse::Resolver resolve)
    : bearer_(bearer),
      registration_(std::move(registration)),
      resolve_(std::move(resolve)),
      nextStream_(kFirstRequestStream) {}

NoiseTransport::~NoiseTransport() { stop(); }

void NoiseTransport::start(const std::string& ssid) {
  if (alive_) return;
  registration_.ssid = ssid;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    fault_ = muse::Fault::None;
  }
  running_ = true;
  alive_ = true;
  if (xTaskCreatePinnedToCore(&NoiseTransport::entry, "noise", kStackSize, this, 3, nullptr, 0) !=
      pdPASS) {
    running_ = false;
    alive_ = false;
  }
}

void NoiseTransport::stop() {
  running_ = false;
  while (alive_) vTaskDelay(pdMS_TO_TICKS(10));
}

void NoiseTransport::entry(void* self) {
  auto* transport = static_cast<NoiseTransport*>(self);
  transport->run();
  transport->alive_ = false;
  vTaskDelete(nullptr);
}

void NoiseTransport::run() {
  muse::Target target;
  bool resolved = false;
  uint32_t backoff = kMinBackoffMs;
  uint8_t shortFailures = 0;

  while (running_) {
    if (!bearer_.online()) {
      sleep(kOfflinePollMs);
      continue;
    }
    if (!resolved) {
      const muse::Resolution resolution = resolve_(target);
      if (resolution == muse::Resolution::Revoked) return raise(muse::Fault::Revoked);
      if (resolution == muse::Resolution::Failed) {
        ESP_LOGW(kTag, "resolve failed, retry in %lu ms", static_cast<unsigned long>(backoff));
        sleep(backoff);
        backoff = std::min(backoff * 2, kMaxBackoffMs);
        continue;
      }
      resolved = true;
    }

    const uint32_t began = nowMs();
    const Outcome outcome = connect(target);
    setRegistered(false);
    if (!running_) break;
    if (outcome == Outcome::Unpaired) return raise(muse::Fault::Unpaired);
    ESP_LOGW(kTag, "session ended (%d) after %lu ms", static_cast<int>(outcome),
             static_cast<unsigned long>(nowMs() - began));

    if (outcome == Outcome::Rejected) {
      resolved = false;
      shortFailures = 0;
      backoff = kAuthBackoffMs;
    } else if (nowMs() - began > kShortSessionMs) {
      shortFailures = 0;
      backoff = kMinBackoffMs;
    } else {
      backoff = std::min(backoff * 2, kMaxBackoffMs);
      if (++shortFailures >= kShortFailuresBeforeRefresh) {
        shortFailures = 0;
        resolved = false;
      }
    }
    sleep(backoff);
  }
}

NoiseTransport::Outcome NoiseTransport::connect(const muse::Target& target) {
  Socket socket(running_);
  if (!socket.connect(target.host, kPort)) {
    ESP_LOGW(kTag, "tls connect to %s failed", target.host.c_str());
    return Outcome::Closed;
  }
  switch (socket.upgrade(target.host, "/v1/noise?vm_id=" + escape(target.vm), target.token)) {
    case Socket::Upgrade::Rejected:
      ESP_LOGW(kTag, "websocket upgrade rejected");
      return Outcome::Rejected;
    case Socket::Upgrade::Failed:
      ESP_LOGW(kTag, "websocket upgrade failed");
      return Outcome::Closed;
    case Socket::Upgrade::Ok:
      break;
  }
  Session session(*this, socket);
  return session.run();
}

void NoiseTransport::sleep(uint32_t ms) {
  for (uint32_t waited = 0; running_ && waited < ms; waited += 100) vTaskDelay(pdMS_TO_TICKS(100));
}

void NoiseTransport::raise(muse::Fault fault) {
  std::lock_guard<std::mutex> lock(mutex_);
  fault_ = fault;
  running_ = false;
}

void NoiseTransport::setRegistered(bool on) {
  if (registered_.exchange(on) == on) return;
  std::deque<Op> dropped;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    transitions_.push_back(on);
    if (on) return;
    results_.clear();
    dropped.swap(ops_);
    queuedBytes_ = 0;
  }
  for (Op& op : dropped) {
    if (op.kind == Op::Kind::Open) op.handler(-1, nullptr, 0, true);
  }
}

void NoiseTransport::loop() {
  std::deque<std::pair<std::string, std::string>> inbound;
  std::deque<bool> transitions;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    inbound.swap(inbound_);
    transitions.swap(transitions_);
  }
  for (bool on : transitions) {
    if (connectionHandler_) connectionHandler_(on);
  }
  for (const auto& message : inbound) {
    if (messageHandler_) messageHandler_(message.first, message.second);
  }
}

void NoiseTransport::publish(const std::string& topic, const std::string& payload) {
  if (!registered_) return;
  std::lock_guard<std::mutex> lock(mutex_);
  results_.push_back(muse::encodeResult(topic, payload));
}

muse::Fault NoiseTransport::fault() {
  std::lock_guard<std::mutex> lock(mutex_);
  return fault_;
}

std::string NoiseTransport::agent() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return agent_;
}

int64_t NoiseTransport::open(const char* verb, const char* path, const muse::Headers& headers,
                            bool end, muse::StreamHandler handler) {
  if (!registered_) return -1;
  Op op{Op::Kind::Open};
  op.stream = nextStream_++;
  op.verb = verb;
  op.path = path;
  op.headers = headers;
  op.end = end;
  op.handler = std::move(handler);
  const int64_t stream = op.stream;
  std::lock_guard<std::mutex> lock(mutex_);
  ops_.push_back(std::move(op));
  return stream;
}

bool NoiseTransport::send(int64_t stream, const std::string& data, bool end) {
  if (!registered_) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  if (queuedBytes_ + data.size() > kMaxQueuedBytes) return false;
  Op op{Op::Kind::Body};
  op.stream = stream;
  op.data = data;
  op.end = end;
  queuedBytes_ += data.size();
  ops_.push_back(std::move(op));
  return true;
}

void NoiseTransport::cancel(int64_t stream) {
  Op op{Op::Kind::Cancel};
  op.stream = stream;
  std::lock_guard<std::mutex> lock(mutex_);
  ops_.push_back(std::move(op));
}

}  // namespace tama

#endif
