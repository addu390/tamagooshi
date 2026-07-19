#pragma once

#include <string>

#include "model.h"

namespace tama::config {

// Wire format shared with tools/gen/emit/blob.py and hub/console/js/wire/blob.js
// (mirrored to docs/js/wire/blob.js by gen): magic "TMG1", uint16 little-endian
// payload length, then that many JSON bytes. Custom themes ride as
// themes: [{name, colors: [9 x uint16]}] with the role order from tools/gen/ui/themes.py.
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
