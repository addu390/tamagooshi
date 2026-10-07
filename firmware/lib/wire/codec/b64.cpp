#include "b64.h"

namespace tama::b64 {

namespace {

constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char kUrlAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

std::string encodeWith(const char* alphabet, bool pad, const uint8_t* data, size_t len) {
  std::string out;
  out.reserve(((len + 2) / 3) * 4);
  size_t i = 0;
  for (; i + 2 < len; i += 3) {
    const uint32_t v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
    out.push_back(alphabet[(v >> 18) & 0x3F]);
    out.push_back(alphabet[(v >> 12) & 0x3F]);
    out.push_back(alphabet[(v >> 6) & 0x3F]);
    out.push_back(alphabet[v & 0x3F]);
  }
  if (i < len) {
    uint32_t v = data[i] << 16;
    const bool two = i + 1 < len;
    if (two) v |= data[i + 1] << 8;
    out.push_back(alphabet[(v >> 18) & 0x3F]);
    out.push_back(alphabet[(v >> 12) & 0x3F]);
    if (two) out.push_back(alphabet[(v >> 6) & 0x3F]);
    if (pad) out.append(two ? "=" : "==");
  }
  return out;
}

int urlValue(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '-') return 62;
  if (c == '_') return 63;
  return -1;
}

}  // namespace

std::string encode(const uint8_t* data, size_t len) { return encodeWith(kAlphabet, true, data, len); }

std::string encodeUrl(const uint8_t* data, size_t len) {
  return encodeWith(kUrlAlphabet, false, data, len);
}

bool decodeUrl(const std::string& text, std::string& out) {
  if (text.size() % 4 == 1) return false;
  out.clear();
  out.reserve(text.size() * 3 / 4);
  uint32_t acc = 0;
  int bits = 0;
  for (char c : text) {
    const int v = urlValue(c);
    if (v < 0) return false;
    acc = (acc << 6) | static_cast<uint32_t>(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<char>((acc >> bits) & 0xFF));
    }
  }
  return true;
}

}  // namespace tama::b64
