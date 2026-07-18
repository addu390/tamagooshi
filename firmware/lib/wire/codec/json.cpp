#include "json.h"

#include <ArduinoJson.h>

#include "fields.h"
#include "types.h"

namespace tama {

namespace {

std::string str(JsonVariantConst v) { return v.is<const char*>() ? v.as<const char*>() : ""; }

bool parseBody(const std::string& body, JsonDocument& doc) {
  return deserializeJson(doc, body) == DeserializationError::Ok;
}

template <typename Fill>
std::string envelope(const char* type, const IdFn& id, const ClockFn& clock,
                     const std::string& src, Fill fill) {
  JsonDocument doc;
  doc[fields::kVersion] = fields::kProtocolVersion;
  doc[fields::kType] = type;
  doc[fields::kId] = id ? id() : std::string{};
  doc[fields::kTs] = clock ? clock() : 0;
  doc[fields::kSrc] = src;
  JsonObject b = doc[fields::kBody].to<JsonObject>();
  fill(b);
  std::string out;
  serializeJson(doc, out);
  return out;
}

}  // namespace

bool ArduinoJsonCodec::decodeEnvelope(const std::string& payload, Envelope& out) const {
  JsonDocument doc;
  if (deserializeJson(doc, payload) != DeserializationError::Ok) return false;
  const int v = doc[fields::kVersion] | 0;
  if (v > fields::kProtocolVersion) return false;
  out.v = v;
  out.type = str(doc[fields::kType]);
  out.id = str(doc[fields::kId]);
  out.ts = doc[fields::kTs].as<uint64_t>();
  out.src = str(doc[fields::kSrc]);
  JsonVariantConst body = doc[fields::kBody];
  out.body.clear();
  if (!body.isNull()) serializeJson(body, out.body);
  return !out.type.empty();
}

bool ArduinoJsonCodec::parseMetric(const std::string& body, Metric& out) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  out.key = str(doc[fields::kKey]);
  out.label = str(doc[fields::kLabel]);
  out.value = str(doc[fields::kValue]);
  out.trend = str(doc[fields::kTrend]);
  out.kind = metricKindFromString(str(doc[fields::kKind]));
  out.ts = doc[fields::kTs] | 0;
  return !out.key.empty() && !out.label.empty();
}

bool ArduinoJsonCodec::parseBranding(const std::string& body, Branding& out) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  const std::string name = str(doc[fields::kName]);
  if (name.empty()) return false;
  out.name = name;
  out.tagline = str(doc[fields::kTagline]);
  out.logo_id = str(doc[fields::kLogoId]);
  return true;
}

bool ArduinoJsonCodec::parseMood(const std::string& body, Mood& mood, std::string& reason) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  const std::string state = str(doc[fields::kState]);
  if (state.empty()) return false;
  mood = moodFromString(state);
  reason = str(doc[fields::kReason]);
  return true;
}

bool ArduinoJsonCodec::parsePage(const std::string& body, Page& out) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  out.id = str(doc[fields::kId]);
  out.title = str(doc[fields::kTitle]);
  if (out.id.empty() || out.title.empty()) return false;
  out.severity = severityFromString(str(doc[fields::kSeverity]));
  out.body = str(doc[fields::kBody]);
  out.source = str(doc[fields::kSource]);
  out.ts = doc[fields::kTs] | 0;
  out.requires_ack = doc[fields::kRequiresAck] | true;
  return true;
}

bool ArduinoJsonCodec::parsePageRef(const std::string& body, std::string& id) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  id = str(doc[fields::kId]);
  return !id.empty();
}

bool ArduinoJsonCodec::parseConfig(const std::string& body, ConfigPatch& out) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  if (!doc[fields::kCarouselSecs].isNull())
    out.carousel_secs = doc[fields::kCarouselSecs].as<uint32_t>();
  if (!doc[fields::kBuzzer].isNull()) out.buzzer_enabled = doc[fields::kBuzzer].as<bool>();
  out.theme = str(doc[fields::kTheme]);
  out.character_id = str(doc[fields::kCharacterId]);
  return true;
}

bool ArduinoJsonCodec::parseTime(const std::string& body, int64_t& epoch, int& tzOffsetMin) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  epoch = doc[fields::kEpoch] | static_cast<int64_t>(0);
  tzOffsetMin = doc[fields::kTzOffset] | 0;
  return epoch > 0 || !doc[fields::kTzOffset].isNull();
}

bool ArduinoJsonCodec::parseExpression(const std::string& body, ExpressionCue& out) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  out.kind = expressionKindFromString(str(doc[fields::kKind]));
  out.intensityPct = doc[fields::kIntensity] | 100;
  out.durationMs = doc[fields::kDurationMs] | 0;
  return true;
}

bool ArduinoJsonCodec::parseCommand(const std::string& body, std::string& cmd) const {
  JsonDocument doc;
  if (!parseBody(body, doc)) return false;
  cmd = str(doc[fields::kCmd]);
  return !cmd.empty();
}

std::string ArduinoJsonCodec::encodeAck(const std::string& pageId) const {
  return envelope(mtype::kPageAck, id_, clock_, src_, [&](JsonObject b) {
    b[fields::kPageId] = pageId;
    b[fields::kBy] = "device";
  });
}

std::string ArduinoJsonCodec::encodeStatus(bool online, const std::string& fw,
                                           const std::string& ip) const {
  return envelope(mtype::kDeviceStatus, id_, clock_, src_, [&](JsonObject b) {
    b[fields::kState] = online ? "online" : "offline";
    b[fields::kFw] = fw;
    b[fields::kIp] = ip;
  });
}

std::string ArduinoJsonCodec::encodeHello(const DeviceCapabilities& caps,
                                          const std::string& fw) const {
  return envelope(mtype::kDeviceHello, id_, clock_, src_, [&](JsonObject b) {
    b[fields::kProto] = fields::kProtocolVersion;
    b[fields::kModel] = caps.model;
    JsonObject screen = b[fields::kScreen].to<JsonObject>();
    screen[fields::kWidth] = caps.screenW;
    screen[fields::kHeight] = caps.screenH;
    JsonObject c = b[fields::kCaps].to<JsonObject>();
    c[fields::kButtons] = caps.buttons;
    c[fields::kLed] = caps.led;
    c[fields::kBuzzer] = caps.buzzer;
    c[fields::kSpeaker] = caps.speaker;
    c[fields::kMic] = caps.mic;
    c[fields::kImu] = caps.imu;
    c[fields::kJoystick] = caps.joystick;
    c[fields::kHaptics] = caps.haptics;
    c[fields::kIr] = caps.ir;
    c[fields::kWearable] = caps.wearable;
    b[fields::kFw] = fw;
  });
}

std::string ArduinoJsonCodec::encodeJoystick(const JoystickEvent& event) const {
  return envelope(mtype::kInputJoystick, id_, clock_, src_, [&](JsonObject b) {
    b[fields::kX] = event.x;
    b[fields::kY] = event.y;
    b[fields::kPressed] = event.pressed;
  });
}

std::string ArduinoJsonCodec::encodeGesture(const GestureEvent& event) const {
  return envelope(mtype::kInputGesture, id_, clock_, src_,
                  [&](JsonObject b) { b[fields::kGesture] = event.name; });
}

std::string ArduinoJsonCodec::encodeMotion(const MotionEvent& event) const {
  return envelope(mtype::kSensorMotion, id_, clock_, src_, [&](JsonObject b) {
    b[fields::kAx] = event.ax;
    b[fields::kAy] = event.ay;
    b[fields::kAz] = event.az;
    b[fields::kEvent] = event.event;
  });
}

}  // namespace tama
