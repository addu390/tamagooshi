#pragma once

#include <cstdint>
#include <string>

#include "radio.h"

namespace tama {

class ILink : public IRadio {
 public:
  virtual std::string deviceName() const = 0;
  virtual std::string deviceId() const = 0;
  virtual uint32_t passkey() const = 0;
  virtual bool paired() const = 0;
  virtual void unpair() = 0;
};

class NullLink : public ILink {
 public:
  bool available() const override { return false; }
  bool enabled() const override { return false; }
  void setEnabled(bool) override {}
  bool connected() const override { return false; }
  std::string peer() const override { return {}; }
  std::string deviceName() const override { return {}; }
  std::string deviceId() const override { return {}; }
  uint32_t passkey() const override { return 0; }
  bool paired() const override { return false; }
  void unpair() override {}
};

}  // namespace tama
