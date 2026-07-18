#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "model.h"

namespace tama {

using IdFn = std::function<std::string()>;
using ClockFn = std::function<uint64_t()>;

struct Envelope {
  int v = 0;
  std::string type;
  std::string id;
  uint64_t ts = 0;
  std::string src;
  std::string body;
};

class ICodec {
 public:
  virtual ~ICodec() = default;

  virtual bool decodeEnvelope(const std::string& payload, Envelope& out) const = 0;

  virtual bool parseMetric(const std::string& body, Metric& out) const = 0;
  virtual bool parseBranding(const std::string& body, Branding& out) const = 0;
  virtual bool parseMood(const std::string& body, Mood& mood, std::string& reason) const = 0;
  virtual bool parsePage(const std::string& body, Page& out) const = 0;
  virtual bool parsePageRef(const std::string& body, std::string& id) const = 0;
  virtual bool parseConfig(const std::string& body, ConfigPatch& out) const = 0;
  virtual bool parseTime(const std::string& body, int64_t& epoch, int& tzOffsetMin) const = 0;
  virtual bool parseExpression(const std::string& body, ExpressionCue& out) const = 0;
  virtual bool parseCommand(const std::string& body, std::string& cmd) const = 0;

  virtual std::string encodeAck(const std::string& pageId) const = 0;
  virtual std::string encodeStatus(bool online, const std::string& fw,
                                   const std::string& ip) const = 0;
  virtual std::string encodeHello(const DeviceCapabilities& caps,
                                  const std::string& fw) const = 0;
  virtual std::string encodeJoystick(const JoystickEvent& event) const = 0;
  virtual std::string encodeGesture(const GestureEvent& event) const = 0;
  virtual std::string encodeMotion(const MotionEvent& event) const = 0;
};

}  // namespace tama
