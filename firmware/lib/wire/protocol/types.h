#pragma once

namespace tama::mtype {

inline constexpr const char* kBrandingSet = "branding.set";
inline constexpr const char* kMetricUpsert = "metric.upsert";
inline constexpr const char* kMoodSet = "mood.set";
inline constexpr const char* kConfigSet = "config.set";
inline constexpr const char* kTimeSet = "time.set";

inline constexpr const char* kPageRaise = "page.raise";
inline constexpr const char* kPageClear = "page.clear";
inline constexpr const char* kPageAck = "page.ack";

inline constexpr const char* kDeviceHello = "device.hello";
inline constexpr const char* kDeviceStatus = "device.status";
inline constexpr const char* kSensorMotion = "sensor.motion";

inline constexpr const char* kInputJoystick = "input.joystick";
inline constexpr const char* kInputGesture = "input.gesture";

inline constexpr const char* kExpressionPlay = "expression.play";

inline constexpr const char* kVoiceSessionStart = "voice.session.start";
inline constexpr const char* kVoiceSessionStop = "voice.session.stop";

inline constexpr const char* kCommandExec = "command.exec";

}  // namespace tama::mtype
