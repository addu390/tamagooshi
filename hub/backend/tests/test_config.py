import pytest
from pydantic import ValidationError
from src.config import (
    BrandNotFound,
    default_catalog,
    hub_config_from_manifest,
    load_config,
)


def test_gooshi_manifest_maps_to_hub_config():
    cfg = hub_config_from_manifest(default_catalog().manifest("gooshi"))

    assert cfg.device_id == "sim"
    assert cfg.brand.name == "TAMAGOOSHI"
    assert cfg.brand.tagline == "keep it alive"
    assert cfg.brand.logo_id == "gooshi"
    assert cfg.brand.theme == "midnight"
    assert cfg.brand.mascot == "jolly"


def test_manifest_rules_validate():
    manifest = {
        "brand": {"id": "acme", "name": "ACME"},
        "device": {
            "moods": [{"when": {"metric": "uptime", "op": "lt", "value": 95}, "mood": "panic",
                       "priority": 20}],
            "alerts": [{"id": "uptime-critical",
                        "when": {"metric": "uptime", "op": "lt", "value": 95},
                        "severity": "critical", "title": "Uptime dropping"}],
        },
    }
    cfg = hub_config_from_manifest(manifest)

    assert cfg.brand_id == "acme"


def test_manifest_rejects_invalid_device_rule():
    bad = {"brand": {"name": "BAD"},
           "device": {"moods": [{"when": {"metric": "m", "op": "lt", "value": 1},
                                 "mood": "grumpy"}]}}
    with pytest.raises(ValidationError):
        hub_config_from_manifest(bad)


def test_manifest_without_hub_is_valid():
    cfg = hub_config_from_manifest({"brand": {"name": "BARE"}})
    assert cfg.brand.name == "BARE"


def test_device_id_env_override(monkeypatch):
    monkeypatch.setenv("TAMA_BRAND", "gooshi")
    monkeypatch.setenv("TAMA_DEVICE_ID", "totem-1")
    cfg = load_config()
    assert cfg.device_id == "totem-1"


def test_unknown_brand_raises(monkeypatch):
    monkeypatch.delenv("TAMA_BRANDS_DIR", raising=False)
    with pytest.raises(BrandNotFound):
        default_catalog().manifest("does-not-exist")
