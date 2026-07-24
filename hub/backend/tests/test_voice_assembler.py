import base64

from src.features.buddy.voice import VoiceAssembler
from src.wire.codec import adpcm


def b64(data: bytes) -> str:
    return base64.b64encode(data).decode("ascii")


def test_reassembles_chunks_in_order():
    payload = bytes(range(100))
    assembler = VoiceAssembler()
    assembler.add_chunk(0, b64(payload[:40]))
    assembler.add_chunk(1, b64(payload[40:]))

    assert assembler.finish() == adpcm.decode(payload)


def test_gap_breaks_the_stream():
    assembler = VoiceAssembler()
    assembler.add_chunk(0, b64(b"aa"))
    assembler.add_chunk(2, b64(b"bb"))
    assert assembler.finish() is None


def test_bad_base64_breaks_the_stream():
    assembler = VoiceAssembler()
    assembler.add_chunk(0, "!!!not base64!!!")
    assert assembler.finish() is None


def test_seq_zero_restarts_after_broken_stream():
    assembler = VoiceAssembler()
    assembler.add_chunk(0, b64(b"aa"))
    assembler.add_chunk(5, b64(b"bb"))
    assembler.add_chunk(0, b64(b"cc"))
    assert assembler.finish() == adpcm.decode(b"cc")


def test_finish_resets_for_next_utterance():
    assembler = VoiceAssembler()
    assembler.add_chunk(0, b64(b"aa"))
    assert assembler.finish() is not None
    assert assembler.finish() is None
