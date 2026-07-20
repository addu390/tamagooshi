#pragma once

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
  const char* note;
};

inline bool locked(const FeatureInfo& info, const DeviceCapabilities& caps) {
  if (info.screen == nullptr) return true;
  if (info.needsJoystick && !caps.joystick) return true;
  if (info.needsImu && !caps.imu) return true;
  if (info.needsMic && !caps.mic) return true;
  if (info.needsIr && !caps.ir) return true;
  return false;
}

}  // namespace tama
