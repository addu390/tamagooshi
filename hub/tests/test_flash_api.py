import asyncio
import base64
import json
import time
from types import SimpleNamespace

import pytest
from fastapi import HTTPException

from src.api.flash import RELEASE_PREFIX, flash_status, start_flash
from src.services.flash import Flasher

IMAGE_URL = RELEASE_PREFIX + "gooshi-m5stickcplus.bin"


class StubFlasher:
    def __init__(self, busy=False):
        self.busy = busy
        self.calls = []

    def start(self, port, image_url, config_offset=None, config_blob=None):
        if self.busy:
            raise RuntimeError("a flash is already running")
        self.calls.append((port, image_url, config_offset, config_blob))
        return "job1234"


def _request(body=None, flasher=None):
    async def json_body():
        return json.loads(json.dumps(body))

    state = SimpleNamespace(flasher=flasher or StubFlasher())
    return SimpleNamespace(app=SimpleNamespace(state=state), json=json_body)


def test_status_reports_ports_and_busy(monkeypatch):
    monkeypatch.setattr("src.services.flash.available", lambda: True)
    monkeypatch.setattr("src.services.flash.list_ports",
                        lambda: [{"device": "/dev/cu.usb1", "description": "USB Serial"}])

    body = asyncio.run(flash_status(_request(flasher=StubFlasher(busy=True))))

    assert body == {"available": True,
                    "ports": [{"device": "/dev/cu.usb1", "description": "USB Serial"}],
                    "busy": True}


def test_start_requires_port():
    with pytest.raises(HTTPException) as err:
        asyncio.run(start_flash(_request({"image_url": IMAGE_URL})))
    assert err.value.status_code == 400


def test_start_rejects_foreign_image_url():
    with pytest.raises(HTTPException) as err:
        asyncio.run(start_flash(_request(
            {"port": "/dev/cu.usb1", "image_url": "https://evil.example/fw.bin"})))
    assert err.value.status_code == 400


def test_start_rejects_bad_base64():
    with pytest.raises(HTTPException) as err:
        asyncio.run(start_flash(_request(
            {"port": "/dev/cu.usb1", "image_url": IMAGE_URL,
             "config_offset": 100, "config_b64": "not base64!!"})))
    assert err.value.status_code == 400


def test_start_requires_offset_with_config():
    with pytest.raises(HTTPException) as err:
        asyncio.run(start_flash(_request(
            {"port": "/dev/cu.usb1", "image_url": IMAGE_URL,
             "config_b64": base64.b64encode(b"TMG1").decode()})))
    assert err.value.status_code == 400


def test_start_passes_decoded_blob():
    flasher = StubFlasher()
    blob = b"TMG1\x02\x00{}"

    body = asyncio.run(start_flash(_request(
        {"port": "/dev/cu.usb1", "image_url": IMAGE_URL,
         "config_offset": 0x310000, "config_b64": base64.b64encode(blob).decode()},
        flasher=flasher)))

    assert body == {"job": "job1234"}
    assert flasher.calls == [("/dev/cu.usb1", IMAGE_URL, 0x310000, blob)]


def test_start_busy_maps_to_409():
    with pytest.raises(HTTPException) as err:
        asyncio.run(start_flash(_request(
            {"port": "/dev/cu.usb1", "image_url": IMAGE_URL},
            flasher=StubFlasher(busy=True))))
    assert err.value.status_code == 409


def test_flasher_single_flight_and_events(monkeypatch):
    events = []
    flasher = Flasher(events.append)

    def fake_run(self, job, port, image_url, config_offset, config_blob):
        self._emit(job, "flashing", pct=50)
        self._emit(job, "finished", pct=100)
        with self._lock:
            self._job = None

    monkeypatch.setattr(Flasher, "_run", fake_run)
    job = flasher.start("/dev/cu.usb1", IMAGE_URL)

    for _ in range(100):
        if not flasher.busy:
            break
        time.sleep(0.01)

    assert not flasher.busy
    assert [e["state"] for e in events] == ["flashing", "finished"]
    assert all(e["job"] == job for e in events)
