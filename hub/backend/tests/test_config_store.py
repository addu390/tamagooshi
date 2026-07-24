import pytest
from pydantic import ValidationError
from src.config import BrandService, default_catalog


@pytest.fixture
def store(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    return BrandService(default_catalog())


def test_update_identity_sets_and_clears_fields(store):
    store.update_identity("demo", {"name": "ACME", "tagline": "", "website": "acme.dev"})

    brand = store.read_manifest("demo")["brand"]
    assert brand["id"] == "demo"
    assert brand["name"] == "ACME"
    assert brand["website"] == "acme.dev"
    assert "tagline" not in brand


def test_update_identity_ignores_unknown_fields(store):
    store.update_identity("demo", {"id": "hacked", "name": "ACME"})

    assert store.read_manifest("demo")["brand"]["id"] == "demo"


def test_update_device_replaces_section(store):
    device = store.read_manifest("demo").get("device") or {}
    device["carousel_secs"] = 42
    store.update_device("demo", device)

    assert store.read_manifest("demo")["device"]["carousel_secs"] == 42


def test_update_rules_replaces_lists(store):
    moods = [{"when": {"metric": "mrr", "op": "gte", "value": 1}, "mood": "celebrate"}]
    alerts = [{"id": "a1", "when": {"metric": "mrr", "op": "lt", "value": 1},
               "severity": "critical", "title": "MRR dropped"}]
    store.update_rules("demo", moods=moods, alerts=alerts)

    hub = store.read_manifest("demo")["hub"]
    assert hub["moods"] == moods
    assert hub["alerts"] == alerts


def test_update_rules_partial_leaves_other_list(store):
    before = store.read_manifest("demo")["hub"].get("alerts") or []
    store.update_rules("demo", moods=[])

    hub = store.read_manifest("demo")["hub"]
    assert hub["moods"] == []
    assert (hub.get("alerts") or []) == before


def test_update_agent(store):
    store.update_agent("demo", "claude", ["claude", "cursor"])

    agent = store.read_manifest("demo")["hub"]["agent"]
    assert agent == {"default": "claude", "enabled": ["claude", "cursor"]}


def test_invalid_mutation_not_written(store, tmp_path):
    with pytest.raises(ValidationError):
        store.update_rules("demo", moods=[{"when": {"metric": "m", "op": "lt", "value": 1},
                                          "mood": "not-a-mood"}])

    assert not (tmp_path / "brands" / "demo.yaml").exists()
