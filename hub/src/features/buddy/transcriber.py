from __future__ import annotations

import abc
import logging

log = logging.getLogger("tamagooshi.buddy.transcriber")


class Transcriber(abc.ABC):
    @abc.abstractmethod
    def transcribe(self, pcm: bytes, sample_rate: int) -> str:
        """Turns 16-bit little-endian mono PCM into text. Blocking; run off the event loop."""


class WhisperTranscriber(Transcriber):
    """Local speech-to-text via faster-whisper. No network, no keys."""

    def __init__(self, model_name: str = "base"):
        self._model_name = model_name
        self._model = None

    def transcribe(self, pcm: bytes, sample_rate: int) -> str:
        import numpy as np

        audio = np.frombuffer(pcm, dtype=np.int16).astype(np.float32) / 32768.0
        segments, _info = self._load().transcribe(audio, language=None, beam_size=5)
        return " ".join(segment.text.strip() for segment in segments).strip()

    def _load(self):
        if self._model is None:
            from faster_whisper import WhisperModel

            log.info("loading whisper model '%s'", self._model_name)
            self._model = WhisperModel(self._model_name, device="auto", compute_type="int8")
        return self._model
