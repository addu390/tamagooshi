import yaml

from src.config import default_catalog, load_config
from src.config.settings import load_settings, save_settings


def test_settings_roundtrip(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    assert load_settings() == {}
    save_settings({"brand": "demo"})
    assert load_settings() == {"brand": "demo"}


def test_settings_brand_used_when_env_unset(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    monkeypatch.delenv("TAMA_BRAND", raising=False)
    monkeypatch.delenv("TAMA_BROKER", raising=False)
    save_settings({"brand": "demo"})
    cfg = load_config()
    assert cfg.brand_id == "demo"


def test_env_brand_beats_settings(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    monkeypatch.setenv("TAMA_BRAND", "gooshi")
    save_settings({"brand": "demo"})
    cfg = load_config()
    assert cfg.brand_id == "gooshi"


def test_user_brand_dir_wins(tmp_path, monkeypatch):
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path))
    user_brands = tmp_path / "brands"
    user_brands.mkdir()
    (user_brands / "gooshi.yaml").write_text(
        yaml.safe_dump({"brand": {"id": "gooshi", "name": "CUSTOM"}}), encoding="utf-8"
    )
    catalog = default_catalog()
    assert catalog.origin("gooshi") == "user"
    assert catalog.manifest("gooshi")["brand"]["name"] == "CUSTOM"
