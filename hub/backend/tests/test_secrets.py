import os
import stat

from src.config import BrandService, default_catalog
from src.config.secrets import apply_secrets, delete_secret, load_secrets, set_secret
from src.config.settings import data_dir


def test_secret_roundtrip(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    monkeypatch.delenv("MY_KEY", raising=False)

    set_secret("MY_KEY", "hunter2")
    assert load_secrets() == {"MY_KEY": "hunter2"}
    assert os.environ["MY_KEY"] == "hunter2"

    mode = stat.S_IMODE(os.stat(os.path.join(data_dir(), "secrets.env")).st_mode)
    assert mode == 0o600

    delete_secret("MY_KEY")
    assert load_secrets() == {}
    assert "MY_KEY" not in os.environ


def test_apply_secrets_respects_real_env(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    set_secret("SHADOWED", "from-file")
    monkeypatch.setenv("SHADOWED", "from-env")
    apply_secrets()
    assert os.environ["SHADOWED"] == "from-env"


def test_store_copy_on_write_for_builtin_brand(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    store = BrandService(default_catalog())

    store.set_source_enabled("demo", 0, False)

    user_copy = tmp_path / "brands" / "demo.yaml"
    assert user_copy.exists()
    data = store.read_manifest("demo")
    assert data["hub"]["sources"][0]["enabled"] is False
    assert data["brand"]["id"] == "demo"


def test_store_add_and_remove_source(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    store = BrandService(default_catalog())

    before = len(store.read_manifest("demo")["hub"]["sources"])
    store.add_source("demo", {"type": "demo", "metrics": []})
    assert len(store.read_manifest("demo")["hub"]["sources"]) == before + 1

    store.remove_source("demo", before)
    assert len(store.read_manifest("demo")["hub"]["sources"]) == before


def test_store_update_source(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    store = BrandService(default_catalog())

    replacement = {"type": "demo", "interval_secs": 9.0,
                   "metrics": [{"key": "swapped", "label": "SWAPPED", "value_start": 1.0}]}
    store.update_source("demo", 0, replacement)

    saved = store.read_manifest("demo")["hub"]["sources"][0]
    assert saved == replacement
