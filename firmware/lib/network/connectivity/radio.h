#pragma once

#include <string>

namespace tama {

class IRadio {
 public:
  virtual ~IRadio() = default;
  virtual bool available() const = 0;
  virtual bool enabled() const = 0;
  virtual void setEnabled(bool on) = 0;
  virtual bool connected() const = 0;
  virtual std::string peer() const = 0;
};

}  // namespace tama
