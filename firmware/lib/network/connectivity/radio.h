#pragma once

#include <optional>
#include <string>

namespace tama {

class IRadioStateRepository {
 public:
  virtual ~IRadioStateRepository() = default;
  virtual std::optional<bool> enabled() const = 0;
  virtual void setEnabled(bool on) = 0;
};

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
