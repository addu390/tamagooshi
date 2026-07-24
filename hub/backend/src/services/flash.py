from __future__ import annotations

import re
import subprocess
import sys
import tempfile
import threading
import uuid
from collections.abc import Callable
from pathlib import Path

import httpx

try:
    from serial.tools import list_ports as _list_ports
except ImportError:  # pyserial arrives with esptool; absent in slim installs
    _list_ports = None

ProgressHandler = Callable[[dict], None]

_PERCENT = re.compile(r"(\d{1,3})\s*%")
_BAUD = "460800"


def available() -> bool:
    return _list_ports is not None


def list_ports() -> list[dict]:
    if _list_ports is None:
        return []
    return [{"device": p.device, "description": p.description}
            for p in _list_ports.comports() if p.vid is not None]


class Flasher:
    def __init__(self, publish: ProgressHandler):
        self._publish = publish
        self._lock = threading.Lock()
        self._job: str | None = None

    @property
    def busy(self) -> bool:
        return self._job is not None

    def start(self, port: str, image_url: str,
              config_offset: int | None = None,
              config_blob: bytes | None = None) -> str:
        with self._lock:
            if self._job is not None:
                raise RuntimeError("a flash is already running")
            self._job = uuid.uuid4().hex[:8]

        job = self._job
        thread = threading.Thread(target=self._run, name="flash",
                                  args=(job, port, image_url, config_offset, config_blob),
                                  daemon=True)
        thread.start()
        return job

    def _emit(self, job: str, state: str, pct: int | None = None,
              message: str | None = None) -> None:
        data: dict = {"job": job, "state": state}
        if pct is not None:
            data["pct"] = pct
        if message is not None:
            data["message"] = message
        self._publish(data)

    def _run(self, job: str, port: str, image_url: str,
             config_offset: int | None, config_blob: bytes | None) -> None:
        try:
            with tempfile.TemporaryDirectory(prefix="tama-flash-") as tmp:
                parts = self._prepare(job, Path(tmp), image_url, config_offset, config_blob)
                self._write(job, port, parts)
            self._emit(job, "finished", pct=100, message="Done. The device restarts itself.")
        except Exception as err:  # noqa: BLE001
            self._emit(job, "error", message=str(err))
        finally:
            with self._lock:
                self._job = None

    def _prepare(self, job: str, tmp: Path, image_url: str,
                 config_offset: int | None,
                 config_blob: bytes | None) -> list[tuple[int, Path]]:
        self._emit(job, "downloading", pct=0, message="Downloading firmware image…")

        image = tmp / "firmware.bin"
        with httpx.stream("GET", image_url, follow_redirects=True, timeout=120) as resp:
            resp.raise_for_status()
            with open(image, "wb") as fh:
                fh.writelines(resp.iter_bytes())

        parts: list[tuple[int, Path]] = [(0, image)]
        if config_blob is not None and config_offset is not None:
            config = tmp / "config.bin"
            config.write_bytes(config_blob)
            parts.append((config_offset, config))
        return parts

    def _write(self, job: str, port: str, parts: list[tuple[int, Path]]) -> None:
        cmd = [sys.executable, "-m", "esptool", "--port", port, "--baud", _BAUD,
               "write-flash"]
        for offset, path in parts:
            cmd += [hex(offset), str(path)]

        self._emit(job, "flashing", pct=0, message="Writing flash…")
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                text=True)
        assert proc.stdout is not None
        for line in proc.stdout:
            match = _PERCENT.search(line)
            if match:
                self._emit(job, "flashing", pct=min(int(match.group(1)), 100))

        if proc.wait() != 0:
            raise RuntimeError(f"esptool exited with code {proc.returncode}")
