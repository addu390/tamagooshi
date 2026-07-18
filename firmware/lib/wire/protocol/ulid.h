#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace tama::ids {

using RandomByteFn = std::function<uint8_t()>;

inline std::string ulid(uint64_t timeMs, const RandomByteFn& rnd) {
  static constexpr char kCrockford[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
  char out[26];

  for (int i = 9; i >= 0; --i) {
    out[i] = kCrockford[timeMs & 0x1F];
    timeMs >>= 5;
  }

  uint8_t bytes[10];
  for (auto& b : bytes) b = rnd ? rnd() : 0;

  uint32_t acc = 0;
  int bits = 0;
  int bytePos = 0;
  for (int idx = 10; idx < 26; ++idx) {
    while (bits < 5) {
      acc = (acc << 8) | bytes[bytePos++];
      bits += 8;
    }
    bits -= 5;
    out[idx] = kCrockford[(acc >> bits) & 0x1F];
  }

  return std::string(out, sizeof(out));
}

using ClockMsFn = std::function<uint64_t()>;

inline std::function<std::string()> ulidGenerator(ClockMsFn clockMs, RandomByteFn rnd) {
  return [clockMs = std::move(clockMs), rnd = std::move(rnd)]() {
    return ulid(clockMs ? clockMs() : 0, rnd);
  };
}

}  // namespace tama::ids
