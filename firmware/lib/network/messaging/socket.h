#pragma once
#if defined(TAMA_AGENT_MUSE)

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

struct esp_tls;

namespace tama {

class Socket {
 public:
  enum class Upgrade { Ok, Failed, Rejected };

  static constexpr int kClosed = -1;
  static constexpr int kIdle = -2;
  static constexpr int kControl = -3;

  explicit Socket(const std::atomic<bool>& running) : running_(running) {}
  ~Socket() { close(); }

  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  bool connect(const std::string& host, int port);
  Upgrade upgrade(const std::string& host, const std::string& target, const std::string& token);
  bool send(const uint8_t* payload, size_t len);
  bool ping();
  int recv(uint8_t* buf, size_t cap, bool wait);
  void idle(uint32_t ms);
  void close();

 private:
  bool write(const void* data, size_t len);
  bool read(uint8_t* out, size_t len, int64_t deadline);
  bool frame(uint8_t opcode, const uint8_t* payload, size_t len);

  const std::atomic<bool>& running_;
  esp_tls* tls_ = nullptr;
};

}  // namespace tama

#endif
