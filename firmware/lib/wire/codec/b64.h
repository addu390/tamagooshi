#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace tama::b64 {

std::string encode(const uint8_t* data, size_t len);

std::string encodeUrl(const uint8_t* data, size_t len);
bool decodeUrl(const std::string& text, std::string& out);

}  // namespace tama::b64
