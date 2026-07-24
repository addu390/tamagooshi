import pytest
from src.config import (
    BrandNotFound,
    default_catalog,
    hub_config_from_manifest,
    load_config,
)
from src.services.sources import parse_sources
from src.services.sources.datadog import DatadogSourceConfig
from src.services.sources.posthog import PosthogSourceConfig


def test_parse_sources_dispatches_by_type():
    parsed = parse_sources([
        {"type": "datadog", "metrics": [{"key": "cpu", "label": "CPU", "query": "avg:c{*}"}]},
        {"type": "posthog", "project_id": "42",
         "metrics": [{"key": "s", "label": "S", "query": "SELECT 1"}]},
    ])
    assert isinstance(parsed[0], DatadogSourceConfig)
    assert isinstance(parsed[1], PosthogSourceConfig)
    assert parsed[1].project_id == "42"


def test_parse_sources_unknown_type_raises():
    with pytest.raises(ValueError, match="unknown source type"):
        parse_sources([{"type": "splunk"}])


def test_gooshi_manifest_maps_to_hub_config():
    cfg = hub_config_from_manifest(default_catalog().manifest("gooshi"))

    assert cfg.device_id == "sim"
    assert cfg.brand.name == "TAMAGOOSHI"
    assert cfg.brand.tagline == "keep it alive"
    assert cfg.brand.logo_id == "gooshi"
    assert cfg.brand.theme == "midnight"
    assert cfg.brand.mascot == "cat"
    assert cfg.default_mood == "happy"
    assert cfg.sources == []
    assert cfg.moods == []
    assert cfg.alerts == []


def test_demo_manifest_has_demo_hub():
    cfg = hub_config_from_manifest(default_catalog().manifest("demo"))

    assert cfg.brand.logo_id == "demo"
    assert any(r.mood == "panic" for r in cfg.moods)
    assert cfg.alerts[0].id == "uptime-critical"


def test_load_config_reads_sources(monkeypatch):
    monkeypatch.setenv("TAMA_BRAND", "demo")
    monkeypatch.delenv("TAMA_BROKER", raising=False)
    monkeypatch.delenv("TAMA_DEVICE_ID", raising=False)
    cfg = load_config()
    assert [s.type for s in cfg.sources] == ["demo"]
    assert [m.key for m in cfg.sources[0].metrics] == ["mrr", "signups", "uptime"]


def test_manifest_without_hub_has_no_sources_or_rules():
    cfg = hub_config_from_manifest({"brand": {"name": "BARE"}})
    assert cfg.brand.name == "BARE"
    assert cfg.sources == []
    assert cfg.moods == []
    assert cfg.alerts == []


def test_device_id_env_override(monkeypatch):
    monkeypatch.setenv("TAMA_BRAND", "gooshi")
    monkeypatch.setenv("TAMA_DEVICE_ID", "totem-1")
    cfg = load_config()
    assert cfg.device_id == "totem-1"


def test_broker_env_overrides_manifest(monkeypatch):
    monkeypatch.setenv("TAMA_BRAND", "gooshi")
    monkeypatch.setenv("TAMA_BROKER", "broker.example:1884")
    cfg = load_config()
    assert cfg.broker.host == "broker.example"
    assert cfg.broker.port == 1884


def test_unknown_brand_raises(monkeypatch):
    monkeypatch.delenv("TAMA_BRANDS_DIR", raising=False)
    with pytest.raises(BrandNotFound):
        default_catalog().manifest("does-not-exist")
