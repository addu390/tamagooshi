#pragma once

#include <string>

#include "model.h"

namespace tama::config {

// Wire format shared with tools/gen/emit/blob.py and docs/js/config-builder.js:
// magic "TMG1", uint16 little-endian payload length, then that many JSON bytes.
inline constexpr char kMagic[4] = {'T', 'M', 'G', '1'};
inline constexpr size_t kHeaderSize = 6;

class ISource {
 public:
  virtual ~ISource() = default;
  virtual std::string read() = 0;
};

class NullSource : public ISource {
 public:
  std::string read() override { return {}; }
};

bool apply(const std::string& blob, DeviceState& state);

}  // namespace tama::config
