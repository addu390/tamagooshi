#include "adpcm.h"

namespace tama::adpcm {

namespace {

constexpr int kIndexTable[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

constexpr int kStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,
    25,    28,    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,
    88,    97,    107,   118,   130,   143,   157,   173,   190,   209,   230,   253,   279,
    307,   337,   371,   408,   449,   494,   544,   598,   658,   724,   796,   876,   963,
    1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,  3327,
    3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

uint8_t encodeSample(State& s, int sample) {
  const int step = kStepTable[s.index];
  int diff = sample - s.predictor;
  uint8_t code = 0;
  if (diff < 0) {
    code = 8;
    diff = -diff;
  }

  int vpdiff = step >> 3;
  if (diff >= step) {
    code |= 4;
    diff -= step;
    vpdiff += step;
  }
  if (diff >= (step >> 1)) {
    code |= 2;
    diff -= step >> 1;
    vpdiff += step >> 1;
  }
  if (diff >= (step >> 2)) {
    code |= 1;
    vpdiff += step >> 2;
  }

  s.predictor = clamp(s.predictor + ((code & 8) ? -vpdiff : vpdiff), -32768, 32767);
  s.index = clamp(s.index + kIndexTable[code], 0, 88);
  return code;
}

}  // namespace

size_t encodedSize(size_t samples) { return (samples + 1) / 2; }

size_t encode(State& state, const int16_t* pcm, size_t samples, uint8_t* out) {
  size_t written = 0;
  for (size_t i = 0; i + 1 < samples; i += 2) {
    const uint8_t lo = encodeSample(state, pcm[i]);
    const uint8_t hi = encodeSample(state, pcm[i + 1]);
    out[written++] = static_cast<uint8_t>(lo | (hi << 4));
  }
  if (samples & 1) {
    out[written++] = encodeSample(state, pcm[samples - 1]);
  }
  return written;
}

}  // namespace tama::adpcm
