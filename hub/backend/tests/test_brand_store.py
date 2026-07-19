import pytest
import yaml

from src.config import store


def _write_brand(folder, bid, name):
    folder.mkdir(parents=True, exist_ok=True)
    (folder / f"{bid}.yaml").write_text(
        yaml.safe_dump({"brand": {"id": bid, "name": name}}), encoding="utf-8"
    )


def _write_template(builtin):
    builtin.mkdir(parents=True, exist_ok=True)
    (builtin / "template.yaml").write_text(yaml.safe_dump({
        "brand": {"id": "template", "name": "YOUR PRODUCT", "mascot": "Buddy"},
        "hub": {"sources": [], "moods": [], "alerts": []},
    }), encoding="utf-8")


@pytest.fixture
def dirs(tmp_path, monkeypatch):
    user = tmp_path / "data" / "brands"
    builtin = tmp_path / "builtin"
    monkeypatch.setenv("TAMA_DATA_DIR", str(tmp_path / "data"))
    monkeypatch.setenv("TAMA_BRANDS_DIR", str(builtin))
    return user, builtin


def test_user_copy_finds_flat_and_folder(dirs):
    user, _ = dirs
    _write_brand(user, "flat", "FLAT")
    folder = user / "nested"
    folder.mkdir(parents=True)
    (folder / "config.yaml").write_text("brand: {id: nested}", encoding="utf-8")

    assert store.user_copy("flat") == str(user / "flat.yaml")
    assert store.user_copy("nested") == str(folder)
    assert store.user_copy("ghost") is None


def test_builtin_exists(dirs):
    _, builtin = dirs
    _write_brand(builtin, "acme", "ACME")

    assert store.builtin_exists("acme")
    assert not store.builtin_exists("ghost")


def test_list_brands_marks_overrides(dirs):
    user, builtin = dirs
    _write_brand(builtin, "acme", "ACME")
    _write_brand(user, "acme", "EDITED")
    _write_brand(user, "custom", "CUSTOM")

    brands = {b["id"]: b for b in store.list_brands()}

    assert brands["acme"]["source"] == "user"
    assert brands["acme"]["overrides_builtin"] is True
    assert brands["acme"]["name"] == "EDITED"
    assert brands["custom"]["overrides_builtin"] is False


def test_create_brand_clones_template(dirs):
    user, builtin = dirs
    _write_template(builtin)

    assert store.create_brand("acme", "ACME", "beep") == "acme"

    data = yaml.safe_load((user / "acme.yaml").read_text(encoding="utf-8"))
    assert data["brand"] == {"id": "acme", "name": "ACME", "tagline": "beep",
                             "mascot": "Buddy"}
    assert data["hub"]["sources"] == []


def test_create_brand_rejects_bad_input(dirs):
    user, builtin = dirs
    _write_template(builtin)
    _write_brand(user, "taken", "TAKEN")

    for bad_id in ("", "ACME", "has space", "-leading"):
        with pytest.raises(ValueError):
            store.create_brand(bad_id, "ACME")
    with pytest.raises(ValueError):
        store.create_brand("acme", "")
    with pytest.raises(ValueError):
        store.create_brand("taken", "AGAIN")


def test_delete_user_brand_removes_flat_and_folder(dirs):
    user, _ = dirs
    _write_brand(user, "flat", "FLAT")
    folder = user / "nested"
    folder.mkdir(parents=True)
    (folder / "config.yaml").write_text("brand: {id: nested}", encoding="utf-8")

    store.delete_user_brand("flat")
    store.delete_user_brand("nested")

    assert not (user / "flat.yaml").exists()
    assert not folder.exists()


def test_delete_user_brand_missing_raises(dirs):
    with pytest.raises(FileNotFoundError):
        store.delete_user_brand("ghost")


def test_import_manifest_requires_id(dirs):
    with pytest.raises(ValueError):
        store.import_manifest({"brand": {"name": "NO ID"}})


def test_import_manifest_writes_user_copy(dirs):
    user, _ = dirs

    assert store.import_manifest({"brand": {"id": "acme", "name": "ACME"}}) == "acme"
    assert (user / "acme.yaml").exists()
