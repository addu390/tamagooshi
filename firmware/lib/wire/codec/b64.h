#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tama::b64 {

std::string encode(const uint8_t* data, size_t len);

}  // namespace tama::b64
