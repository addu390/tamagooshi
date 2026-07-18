import math
import struct

from src.wire.codec import adpcm


def pcm_sine(samples: int, freq: float = 440.0, rate: int = 16000, amp: int = 12000) -> bytes:
    values = [int(amp * math.sin(2 * math.pi * freq * i / rate)) for i in range(samples)]
    return struct.pack(f"<{samples}h", *values)


def unpack(pcm: bytes) -> list[int]:
    return list(struct.unpack(f"<{len(pcm) // 2}h", pcm))


def test_roundtrip_tracks_signal():
    original = pcm_sine(1600)
    encoded = adpcm.encode(original)
    decoded = adpcm.decode(encoded)

    assert len(encoded) == len(original) // 4
    src, out = unpack(original), unpack(decoded)
    assert len(out) == len(src)

    # Skip the adaptation ramp, then require the decoded wave to stay close.
    err = [abs(a - b) for a, b in zip(src[200:], out[200:])]
    assert sum(err) / len(err) < 500


def test_silence_stays_silent():
    silence = bytes(2 * 400)
    decoded = adpcm.decode(adpcm.encode(silence))
    assert all(abs(v) < 40 for v in unpack(decoded))


def test_streamed_encode_matches_one_shot():
    original = pcm_sine(1024)
    state = adpcm.State()
    streamed = b"".join(
        adpcm.encode(original[off:off + 256], state) for off in range(0, len(original), 256)
    )
    assert streamed == adpcm.encode(original)


def test_odd_sample_count_appends_half_byte():
    original = pcm_sine(101)
    assert len(adpcm.encode(original)) == 51
