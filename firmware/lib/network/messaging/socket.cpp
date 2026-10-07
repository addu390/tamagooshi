#include "socket.h"
#if defined(TAMA_AGENT_MUSE)

#include <esp_crt_bundle.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <esp_tls.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>

#include <cstring>

namespace tama {

namespace {

constexpr int64_t kIoTimeoutUs = 15000000;
constexpr int kConnectTimeoutMs = 15000;
constexpr size_t kMaxResponseHead = 4096;
constexpr size_t kMaxControlPayload = 125;
constexpr uint8_t kBinary = 0x2;
constexpr uint8_t kClose = 0x8;
constexpr uint8_t kPing = 0x9;
constexpr uint8_t kPong = 0xA;

bool pending(ssize_t n) { return n == ESP_TLS_ERR_SSL_WANT_READ || n == ESP_TLS_ERR_SSL_WANT_WRITE; }

int64_t deadlineFromNow() { return esp_timer_get_time() + kIoTimeoutUs; }

int statusCode(const std::string& head) {
  if (head.size() < 12 || head.compare(0, 5, "HTTP/") != 0) return -1;
  int code = 0;
  for (int i = 9; i < 12; ++i) {
    if (head[i] < '0' || head[i] > '9') return -1;
    code = code * 10 + (head[i] - '0');
  }
  return code;
}

}  // namespace

bool Socket::connect(const std::string& host, int port) {
  close();
  tls_ = esp_tls_init();
  if (!tls_) return false;
  esp_tls_cfg_t cfg = {};
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  cfg.timeout_ms = kConnectTimeoutMs;
  if (esp_tls_conn_new_sync(host.c_str(), static_cast<int>(host.size()), port, &cfg, tls_) != 1) {
    close();
    return false;
  }
  int fd = -1;
  if (esp_tls_get_conn_sockfd(tls_, &fd) != ESP_OK || fd < 0) {
    close();
    return false;
  }
  lwip_fcntl(fd, F_SETFL, lwip_fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  return true;
}

Socket::Upgrade Socket::upgrade(const std::string& host, const std::string& target,
                                const std::string& token) {
  if (token.find_first_of("\r\n") != std::string::npos ||
      target.find_first_of("\r\n") != std::string::npos ||
      host.find_first_of("\r\n") != std::string::npos) {
    return Upgrade::Failed;
  }
  const std::string request = "GET " + target + " HTTP/1.1\r\nHost: " + host +
                              "\r\nAuthorization: Bearer " + token +
                              "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                              "Sec-WebSocket-Version: 13\r\n"
                              "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
  if (!write(request.data(), request.size())) return Upgrade::Failed;

  std::string head;
  const int64_t deadline = deadlineFromNow();
  char buf[256];
  while (running_ && esp_timer_get_time() < deadline) {
    const ssize_t n = esp_tls_conn_read(tls_, buf, sizeof(buf));
    if (pending(n)) {
      vTaskDelay(1);
      continue;
    }
    if (n <= 0) return Upgrade::Failed;
    head.append(buf, static_cast<size_t>(n));
    const bool complete = head.find("\r\n\r\n") != std::string::npos;
    if (!complete && head.size() < kMaxResponseHead) continue;
    const int status = statusCode(head);
    if (status == 401 || status == 403) return Upgrade::Rejected;
    return complete && status == 101 ? Upgrade::Ok : Upgrade::Failed;
  }
  return Upgrade::Failed;
}

bool Socket::send(const uint8_t* payload, size_t len) { return frame(kBinary, payload, len); }

bool Socket::ping() { return frame(kPing, nullptr, 0); }

bool Socket::frame(uint8_t opcode, const uint8_t* payload, size_t len) {
  uint8_t head[14];
  size_t headLen = 2;
  head[0] = 0x80 | opcode;
  if (len < 126) {
    head[1] = 0x80 | static_cast<uint8_t>(len);
  } else if (len <= 0xFFFF) {
    head[1] = 0x80 | 126;
    head[2] = static_cast<uint8_t>(len >> 8);
    head[3] = static_cast<uint8_t>(len);
    headLen = 4;
  } else {
    head[1] = 0x80 | 127;
    for (int i = 0; i < 8; ++i) head[2 + i] = static_cast<uint8_t>(uint64_t(len) >> (56 - 8 * i));
    headLen = 10;
  }
  const uint32_t key = esp_random();
  std::memcpy(head + headLen, &key, 4);
  headLen += 4;
  if (!write(head, headLen)) return false;

  const auto* mask = reinterpret_cast<const uint8_t*>(&key);
  uint8_t chunk[512];
  for (size_t off = 0; off < len;) {
    const size_t n = len - off < sizeof(chunk) ? len - off : sizeof(chunk);
    for (size_t i = 0; i < n; ++i) chunk[i] = payload[off + i] ^ mask[(off + i) & 3];
    if (!write(chunk, n)) return false;
    off += n;
  }
  return true;
}

int Socket::recv(uint8_t* buf, size_t cap, bool wait) {
  for (;;) {
    uint8_t head[2];
    const ssize_t first = esp_tls_conn_read(tls_, head, 1);
    if (pending(first)) {
      if (!wait) return kIdle;
      if (!running_) return kClosed;
      vTaskDelay(1);
      continue;
    }
    if (first <= 0) return kClosed;

    const int64_t deadline = deadlineFromNow();
    if (!read(head + 1, 1, deadline)) return kClosed;
    const uint8_t opcode = head[0] & 0x0F;
    const bool masked = head[1] & 0x80;
    uint64_t len = head[1] & 0x7F;
    if (len == 126 || len == 127) {
      uint8_t ext[8];
      const size_t extLen = len == 126 ? 2 : 8;
      if (!read(ext, extLen, deadline)) return kClosed;
      len = 0;
      for (size_t i = 0; i < extLen; ++i) len = (len << 8) | ext[i];
    }
    uint8_t mask[4] = {};
    if (masked && !read(mask, sizeof(mask), deadline)) return kClosed;
    if (len > cap || (opcode >= kClose && len > kMaxControlPayload)) return kClosed;
    const size_t size = static_cast<size_t>(len);
    if (!read(buf, size, deadline)) return kClosed;
    if (masked) {
      for (size_t i = 0; i < size; ++i) buf[i] ^= mask[i & 3];
    }

    if (opcode == kClose) return kClosed;
    if (opcode == kPing) {
      if (!frame(kPong, buf, size)) return kClosed;
    }
    if (opcode == kPing || opcode == kPong) {
      if (!wait) return kControl;
      continue;
    }
    return static_cast<int>(size);
  }
}

void Socket::idle(uint32_t ms) {
  int fd = -1;
  if (!tls_ || esp_tls_get_bytes_avail(tls_) > 0 || esp_tls_get_conn_sockfd(tls_, &fd) != ESP_OK ||
      fd < 0) {
    vTaskDelay(1);
    return;
  }
  fd_set readable;
  FD_ZERO(&readable);
  FD_SET(fd, &readable);
  timeval tv = {static_cast<time_t>(ms / 1000), static_cast<suseconds_t>((ms % 1000) * 1000)};
  lwip_select(fd + 1, &readable, nullptr, nullptr, &tv);
}

void Socket::close() {
  if (tls_) esp_tls_conn_destroy(tls_);
  tls_ = nullptr;
}

bool Socket::write(const void* data, size_t len) {
  const auto* p = static_cast<const uint8_t*>(data);
  const int64_t deadline = deadlineFromNow();
  for (size_t off = 0; off < len;) {
    if (!running_ || esp_timer_get_time() >= deadline) return false;
    const ssize_t n = esp_tls_conn_write(tls_, p + off, len - off);
    if (n > 0) {
      off += static_cast<size_t>(n);
    } else if (pending(n)) {
      vTaskDelay(1);
    } else {
      return false;
    }
  }
  return true;
}

bool Socket::read(uint8_t* out, size_t len, int64_t deadline) {
  for (size_t have = 0; have < len;) {
    if (!running_ || esp_timer_get_time() >= deadline) return false;
    const ssize_t n = esp_tls_conn_read(tls_, out + have, len - have);
    if (n > 0) {
      have += static_cast<size_t>(n);
    } else if (pending(n)) {
      vTaskDelay(1);
    } else {
      return false;
    }
  }
  return true;
}

}  // namespace tama

#endif
