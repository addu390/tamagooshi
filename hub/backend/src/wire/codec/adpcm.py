"""IMA-ADPCM codec, 4 bits per sample.

Mirrors the firmware encoder in firmware/lib/wire/codec/adpcm.cpp; keep them in sync.
"""

from __future__ import annotations

from dataclasses import dataclass

_INDEX_TABLE = (-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8)

_STEP_TABLE = (
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23,
    25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80,
    88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279,
    307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
)


@dataclass
class State:
    predictor: int = 0
    index: int = 0


def _clamp(value: int, lo: int, hi: int) -> int:
    return max(lo, min(hi, value))


def _decode_nibble(state: State, code: int) -> int:
    step = _STEP_TABLE[state.index]
    vpdiff = step >> 3
    if code & 4:
        vpdiff += step
    if code & 2:
        vpdiff += step >> 1
    if code & 1:
        vpdiff += step >> 2

    state.predictor = _clamp(
        state.predictor - vpdiff if code & 8 else state.predictor + vpdiff, -32768, 32767
    )
    state.index = _clamp(state.index + _INDEX_TABLE[code], 0, 88)
    return state.predictor


def _encode_sample(state: State, sample: int) -> int:
    step = _STEP_TABLE[state.index]
    diff = sample - state.predictor
    code = 0
    if diff < 0:
        code = 8
        diff = -diff

    vpdiff = step >> 3
    if diff >= step:
        code |= 4
        diff -= step
        vpdiff += step
    if diff >= step >> 1:
        code |= 2
        diff -= step >> 1
        vpdiff += step >> 1
    if diff >= step >> 2:
        code |= 1
        vpdiff += step >> 2

    state.predictor = _clamp(
        state.predictor - vpdiff if code & 8 else state.predictor + vpdiff, -32768, 32767
    )
    state.index = _clamp(state.index + _INDEX_TABLE[code], 0, 88)
    return code


def decode(data: bytes, state: State | None = None) -> bytes:
    """Decode ADPCM bytes into little-endian 16-bit PCM."""
    state = state or State()
    out = bytearray()
    for byte in data:
        for code in (byte & 0x0F, byte >> 4):
            sample = _decode_nibble(state, code)
            out += sample.to_bytes(2, "little", signed=True)
    return bytes(out)


def encode(pcm: bytes, state: State | None = None) -> bytes:
    """Encode little-endian 16-bit PCM into ADPCM bytes."""
    state = state or State()
    out = bytearray()
    samples = [
        int.from_bytes(pcm[i:i + 2], "little", signed=True) for i in range(0, len(pcm) - 1, 2)
    ]
    for i in range(0, len(samples) - 1, 2):
        lo = _encode_sample(state, samples[i])
        hi = _encode_sample(state, samples[i + 1])
        out.append(lo | (hi << 4))
    if len(samples) % 2:
        out.append(_encode_sample(state, samples[-1]))
    return bytes(out)
