import asyncio
import json
from types import SimpleNamespace

import pytest
from fastapi import HTTPException

from src.api.agents import put_agents
from src.api.config import config as get_config
from src.api.config import put_device, put_identity
from src.api.rules import put_alerts, put_moods
from src.config import store


def _request(body=None, active_brand="demo"):
    config = SimpleNamespace(brand_id=active_brand)

    async def json_body():
        return json.loads(json.dumps(body))

    return SimpleNamespace(app=SimpleNamespace(state=SimpleNamespace(config=config)),
                           json=json_body)


@pytest.fixture
def data_dir(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    monkeypatch.setattr("src.api.lifecycle.schedule_restart", lambda: None)
    return tmp_path


def test_get_config_returns_manifest(data_dir):
    manifest = asyncio.run(get_config(_request()))
    assert manifest["brand"]["id"] == "demo"
    assert "hub" in manifest


def test_put_identity_roundtrip(data_dir):
    result = asyncio.run(put_identity(_request({"name": "ACME", "tagline": "beep"})))

    assert result == {"restarting": True}
    brand = store.read_manifest("demo")["brand"]
    assert brand["name"] == "ACME"
    assert brand["tagline"] == "beep"


def test_put_device_roundtrip(data_dir):
    device = store.read_manifest("demo").get("device") or {}
    device["carousel_secs"] = 12
    asyncio.run(put_device(_request(device)))

    assert store.read_manifest("demo")["device"]["carousel_secs"] == 12


def test_put_device_rejects_non_object(data_dir):
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_device(_request(["not", "a", "dict"])))
    assert err.value.status_code == 400


def test_put_moods_roundtrip(data_dir):
    moods = [{"when": {"metric": "mrr", "op": "gte", "value": 5}, "mood": "celebrate",
              "priority": 3}]
    asyncio.run(put_moods(_request(moods)))

    assert store.read_manifest("demo")["hub"]["moods"] == moods


def test_put_alerts_invalid_rule_400(data_dir):
    bad = [{"id": "x", "when": {"metric": "m", "op": "lt", "value": 1},
            "severity": "nope", "title": "t"}]
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_alerts(_request(bad)))
    assert err.value.status_code == 400


def test_put_rules_rejects_non_list(data_dir):
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_moods(_request({"not": "a list"})))
    assert err.value.status_code == 400


def test_put_agents_roundtrip(data_dir):
    asyncio.run(put_agents(_request({"default": "claude", "enabled": ["claude"]})))

    agent = store.read_manifest("demo")["hub"]["agent"]
    assert agent == {"default": "claude", "enabled": ["claude"]}


def test_put_agents_unknown_agent_400(data_dir):
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_agents(_request({"default": "hal9000", "enabled": ["hal9000"]})))
    assert err.value.status_code == 400


def test_put_agents_default_must_be_enabled(data_dir):
    with pytest.raises(HTTPException) as err:
        asyncio.run(put_agents(_request({"default": "cursor", "enabled": ["claude"]})))
    assert err.value.status_code == 400
