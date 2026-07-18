#pragma once

namespace tama {

class IBearer {
 public:
  virtual ~IBearer() = default;
  virtual void begin() = 0;
  virtual void loop() = 0;
  virtual bool online() const = 0;
};

}  // namespace tama
