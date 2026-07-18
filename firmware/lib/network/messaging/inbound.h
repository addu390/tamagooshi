#pragma once

#include <cstdint>
#include <string>

#include "model.h"

namespace tama {

class IInboundPipeline {
 public:
  virtual ~IInboundPipeline() = default;
  virtual void onConnected(bool connected) { (void)connected; }
  virtual void onInbound(const std::string& topic, const std::string& payload) {
    (void)topic;
    (void)payload;
  }
  virtual void onGesture(const GestureEvent& event) { (void)event; }
  virtual void onMotion(const MotionEvent& event) { (void)event; }
  virtual void tick(uint32_t nowMs) { (void)nowMs; }
};

}  // namespace tama
