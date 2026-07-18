from __future__ import annotations

import base64
import binascii
import logging
from typing import Optional

from ...wire.codec import adpcm

log = logging.getLogger("tamagooshi.buddy.voice")

SAMPLE_RATE = 16000


class VoiceAssembler:
    """Reassembles ADPCM voice chunks streamed from the device.

    The device sends {"cmd":"voice","seq":n,"data":"<b64>"} lines followed by
    {"cmd":"voice_end","ms":...}; seq 0 starts a fresh utterance.
    """

    def __init__(self) -> None:
        self._chunks: list[bytes] = []
        self._next_seq = 0
        self._broken = False

    def add_chunk(self, seq: int, data: str) -> None:
        if seq == 0:
            self.reset()
        if self._broken:
            return
        if seq != self._next_seq:
            log.warning("voice chunk gap: expected seq %d, got %d", self._next_seq, seq)
            self._broken = True
            return
        try:
            self._chunks.append(base64.b64decode(data, validate=True))
        except (binascii.Error, ValueError):
            log.warning("voice chunk %d is not valid base64", seq)
            self._broken = True
            return
        self._next_seq += 1

    def finish(self) -> Optional[bytes]:
        """Returns 16-bit little-endian PCM at SAMPLE_RATE, or None on a broken stream."""
        broken, chunks = self._broken, self._chunks
        self.reset()
        if broken or not chunks:
            return None
        return adpcm.decode(b"".join(chunks))

    def reset(self) -> None:
        self._chunks = []
        self._next_seq = 0
        self._broken = False
