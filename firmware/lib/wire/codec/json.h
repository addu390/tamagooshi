#pragma once

#include <string>
#include <utility>

#include "codec.h"

namespace tama {

class ArduinoJsonCodec : public ICodec {
 public:
  ArduinoJsonCodec(IdFn id, ClockFn clock, std::string src)
      : id_(std::move(id)), clock_(std::move(clock)), src_(std::move(src)) {}

  bool decodeEnvelope(const std::string& payload, Envelope& out) const override;

  bool parseMetric(const std::string& body, Metric& out) const override;
  bool parseBranding(const std::string& body, Branding& out) const override;
  bool parseMood(const std::string& body, Mood& mood, std::string& reason) const override;
  bool parsePage(const std::string& body, Page& out) const override;
  bool parsePageRef(const std::string& body, std::string& id) const override;
  bool parseConfig(const std::string& body, ConfigPatch& out) const override;
  bool parseTime(const std::string& body, int64_t& epoch, int& tzOffsetMin) const override;
  bool parseExpression(const std::string& body, ExpressionCue& out) const override;
  bool parseCommand(const std::string& body, std::string& cmd) const override;

  std::string encodeAck(const std::string& pageId) const override;
  std::string encodeStatus(bool online, const std::string& fw,
                           const std::string& ip) const override;
  std::string encodeHello(const DeviceCapabilities& caps, const std::string& fw) const override;
  std::string encodeJoystick(const JoystickEvent& event) const override;
  std::string encodeGesture(const GestureEvent& event) const override;
  std::string encodeMotion(const MotionEvent& event) const override;

 private:
  IdFn id_;
  ClockFn clock_;
  std::string src_;
};

}  // namespace tama
