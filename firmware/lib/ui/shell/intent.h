#pragma once

namespace tama {

enum class Intent { Select, Next, Prev, Back, Home };

inline int cycleIndex(Intent intent, int cur, int count) {
  if (count <= 0) return cur;
  if (intent == Intent::Next) return (cur + 1) % count;
  if (intent == Intent::Prev) return (cur - 1 + count) % count;
  return cur;
}

}  // namespace tama
