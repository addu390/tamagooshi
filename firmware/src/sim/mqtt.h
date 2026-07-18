#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

#include "topics.h"
#include "transport.h"

namespace tama {

class MqttSimTransport : public ITransport {
 public:
  bool tryConnect(const std::string& hostPort, const std::string& clientId) {
    clientId_ = clientId;
    std::string host = hostPort;
    int port = 1883;
    auto colon = hostPort.find(':');
    if (colon != std::string::npos) {
      host = hostPort.substr(0, colon);
      port = std::atoi(hostPort.c_str() + colon + 1);
    }
    fd_ = openSocket(host, port);
    if (fd_ < 0) return false;
    if (!sendConnect(clientId) || !awaitConnack()) {
      ::close(fd_);
      fd_ = -1;
      return false;
    }
    setNonBlocking();
    connected_ = true;
    lastPing_ = nowMs();
    return true;
  }

  void begin() override {
    if (!connected_) return;
    subscribeTopic(topics::deviceWildcard(clientId_));
    subscribeTopic(topics::fleetWildcard());
    if (connectionHandler_) connectionHandler_(true);
  }

  void loop() override {
    if (!connected_) return;
    pump();
    if (nowMs() - lastPing_ > 20000) {
      const uint8_t ping[] = {0xC0, 0x00};
      sendRaw(reinterpret_cast<const char*>(ping), sizeof(ping));
      lastPing_ = nowMs();
    }
  }

  void publish(const std::string& topic, const std::string& payload, uint8_t, bool retain) override {
    std::string vh;
    putString(vh, topic);
    std::string pkt = vh + payload;
    uint8_t header = 0x30 | (retain ? 0x01 : 0x00);  // QoS 0
    sendPacket(header, pkt);
  }

  bool connected() const override { return connected_; }
  void onMessage(MessageHandler handler) override { messageHandler_ = std::move(handler); }
  void onConnection(ConnectionHandler handler) override {
    connectionHandler_ = std::move(handler);
  }

 private:
  void subscribeTopic(const std::string& topic) {
    std::string vh;
    vh.push_back(static_cast<char>((packetId_ >> 8) & 0xFF));
    vh.push_back(static_cast<char>(packetId_ & 0xFF));
    ++packetId_;
    std::string payload;
    putString(payload, topic);
    payload.push_back(0x00);  // requested QoS 0
    sendPacket(0x82, vh + payload);
  }

  static uint64_t nowMs() {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
  }

  static void putString(std::string& out, const std::string& s) {
    out.push_back(static_cast<char>((s.size() >> 8) & 0xFF));
    out.push_back(static_cast<char>(s.size() & 0xFF));
    out += s;
  }

  static void encodeLen(std::string& out, size_t len) {
    do {
      uint8_t b = len % 128;
      len /= 128;
      if (len) b |= 0x80;
      out.push_back(static_cast<char>(b));
    } while (len);
  }

  int openSocket(const std::string& host, int port) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    char portStr[8];
    std::snprintf(portStr, sizeof(portStr), "%d", port);
    if (getaddrinfo(host.c_str(), portStr, &hints, &res) != 0 || !res) return -1;
    int fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd >= 0 && ::connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
      ::close(fd);
      fd = -1;
    }
    freeaddrinfo(res);
    return fd;
  }

  void setNonBlocking() {
    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);
  }

  bool sendConnect(const std::string& clientId) {
    std::string vh;
    putString(vh, "MQTT");
    vh.push_back(0x04);        // protocol level 4 (3.1.1)
    vh.push_back(0x02);        // clean session
    vh.push_back(0x00);        // keepalive high
    vh.push_back(0x3C);        // keepalive 60s
    std::string payload;
    putString(payload, clientId);
    return sendPacket(0x10, vh + payload);
  }

  bool awaitConnack() {
    char buf[8];
    size_t got = 0;
    auto deadline = nowMs() + 3000;
    while (got < 4 && nowMs() < deadline) {
      ssize_t n = ::recv(fd_, buf + got, sizeof(buf) - got, 0);
      if (n > 0) {
        got += static_cast<size_t>(n);
      } else if (n == 0) {
        return false;
      }
    }
    return got >= 4 && static_cast<uint8_t>(buf[0]) == 0x20 && buf[3] == 0x00;
  }

  bool sendPacket(uint8_t header, const std::string& body) {
    std::string pkt;
    pkt.push_back(static_cast<char>(header));
    encodeLen(pkt, body.size());
    pkt += body;
    return sendRaw(pkt.data(), pkt.size());
  }

  bool sendRaw(const char* data, size_t len) {
    if (fd_ < 0) return false;
    size_t sent = 0;
    while (sent < len) {
      ssize_t n = ::send(fd_, data + sent, len - sent, 0);
      if (n <= 0) return false;
      sent += static_cast<size_t>(n);
    }
    return true;
  }

  void pump() {
    char buf[1024];
    ssize_t n;
    while ((n = ::recv(fd_, buf, sizeof(buf), 0)) > 0) {
      rx_.append(buf, static_cast<size_t>(n));
    }
    parse();
  }

  void parse() {
    size_t offset = 0;
    while (rx_.size() - offset >= 2) {
      size_t i = offset + 1;
      size_t mult = 1, value = 0;
      bool complete = false;
      while (i < rx_.size()) {
        uint8_t b = static_cast<uint8_t>(rx_[i]);
        value += static_cast<size_t>(b & 0x7F) * mult;
        mult *= 128;
        ++i;
        if (!(b & 0x80)) {
          complete = true;
          break;
        }
        if (i - offset > 5) return;  // malformed length
      }
      if (!complete) break;
      size_t total = (i - offset) + value;
      if (rx_.size() - offset < total) break;
      handlePacket(static_cast<uint8_t>(rx_[offset]), rx_.data() + i, value);
      offset += total;
    }
    if (offset) rx_.erase(0, offset);
  }

  void handlePacket(uint8_t header, const char* body, size_t len) {
    if ((header & 0xF0) != 0x30) return;  // only inbound PUBLISH matters
    if (len < 2) return;
    size_t topicLen = (static_cast<uint8_t>(body[0]) << 8) | static_cast<uint8_t>(body[1]);
    if (2 + topicLen > len) return;
    std::string topic(body + 2, topicLen);
    size_t idx = 2 + topicLen;
    uint8_t qos = (header >> 1) & 0x03;
    if (qos > 0) idx += 2;  // packet id (unused; we subscribe QoS 0)
    if (idx > len) return;
    std::string payload(body + idx, len - idx);
    if (messageHandler_) messageHandler_(topic, payload);
  }

  std::string clientId_;
  int fd_ = -1;
  bool connected_ = false;
  uint16_t packetId_ = 1;
  uint64_t lastPing_ = 0;
  std::string rx_;
  MessageHandler messageHandler_;
  ConnectionHandler connectionHandler_;
};

}  // namespace tama
