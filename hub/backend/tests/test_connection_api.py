import asyncio
import json
from types import SimpleNamespace

import pytest
from fastapi import HTTPException

from src.api.routes.connection import connection, forget_device, put_connection
from src.config.settings import load_connection
from src.network.transport.base import LinkStatus
from src.network.transport.factory import transport_spec


class StubTransport:
    def __init__(self):
        self.closed = False

    def status(self):
        return LinkStatus(state="connected", device={"name": "GOOSHI-1", "address": "AA:BB"})

    def close(self):
        self.closed = True


def _request(body=None, transport=None):
    async def json_body():
        return json.loads(json.dumps(body))

    state = SimpleNamespace(transport=transport)
    return SimpleNamespace(app=SimpleNamespace(state=state), json=json_body)


@pytest.fixture
def data_dir(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    monkeypatch.delenv("TAMA_TRANSPORT", raising=False)
    monkeypatch.setattr("src.api.lifecycle.schedule_restart", lambda: None)
    return tmp_path


def test_put_connection_saves_device(data_dir):
    stub = StubTransport()
    result = asyncio.run(put_connection(_request(
        {"transport": "ble", "device": {"name": "GOOSHI-1", "address": "AA:BB"}},
        transport=stub)))

    assert result == {"restarting": True}
    assert stub.closed
    assert load_connection() == {"transport": "ble:gatt",
                                 "device": {"name": "GOOSHI-1", "address": "AA:BB"}}
    assert transport_spec() == "ble:gatt"


def test_put_connection_unknown_link_400(data_dir):
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_connection(_request({"transport": "carrier-pigeon"})))
    assert err.value.status_code == 400


def test_put_connection_rejects_bad_device(data_dir):
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_connection(_request({"transport": "ble", "device": ["nope"]})))
    assert err.value.status_code == 400


def test_forget_device_drops_link(data_dir):
    asyncio.run(put_connection(_request(
        {"transport": "ble", "device": {"name": "G", "address": "AA:BB"}},
        transport=StubTransport())))
    stub = StubTransport()
    asyncio.run(forget_device(_request(transport=stub)))

    assert stub.closed
    assert load_connection() == {"transport": "ble:gatt"}


def test_get_connection_reports_transport(data_dir):
    body = asyncio.run(connection(_request(transport=StubTransport())))

    assert body["link"] == "ble"
    assert body["locked"] is False
    assert body["state"] == "connected"
    assert body["device"]["name"] == "GOOSHI-1"


def test_env_overrides_saved_transport(data_dir, monkeypatch):
    asyncio.run(put_connection(_request({"transport": "ble"})))
    monkeypatch.setenv("TAMA_TRANSPORT", "wifi:mqtt")

    assert transport_spec() == "wifi:mqtt"
