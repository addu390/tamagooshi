#pragma once

#include <cstddef>
#include <cstdint>

namespace tama::adpcm {

// IMA-ADPCM, 4 bits per sample (4:1 over 16-bit PCM). The hub mirrors this
// exact algorithm in hub/src/wire/codec/adpcm.py; keep them in sync.
struct State {
  int predictor = 0;
  int index = 0;
};

size_t encodedSize(size_t samples);
size_t encode(State& state, const int16_t* pcm, size_t samples, uint8_t* out);

}  // namespace tama::adpcm
