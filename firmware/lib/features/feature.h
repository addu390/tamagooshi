#pragma once

#include "hid.h"
#include "model.h"

namespace tama {

struct FeatureInfo {
  const char* id;
  const char* label;
  const char* screen;
  bool needsJoystick;
  bool needsImu;
  bool needsMic;
  bool needsIr;
  bool needsHid;
  HidCapability hid;
  const char* note;
};

inline bool locked(const FeatureInfo& info, const DeviceCapabilities& caps,
                   HidCapabilitySet hidProfile) {
  if (info.screen == nullptr) return true;
  if (info.needsJoystick && !caps.joystick) return true;
  if (info.needsImu && !caps.imu) return true;
  if (info.needsMic && !caps.mic) return true;
  if (info.needsIr && !caps.ir) return true;
  if (info.needsHid && !hidProfile.has(info.hid)) return true;
  return false;
}

inline const char* lockNote(const FeatureInfo& info, const DeviceCapabilities& caps,
                            HidCapabilitySet hidProfile) {
  if (info.note) return info.note;
  if (info.needsHid && !hidProfile.has(info.hid)) return "MODE";
  if (locked(info, caps, hidProfile)) return "LOCKED";
  return nullptr;
}

}  // namespace tama
