import asyncio
import json
from types import SimpleNamespace

import pytest
import yaml
from fastapi import HTTPException
from src.api.routes.brands import (
    brand_manifest,
    create_brand,
    delete_brand,
    export_brand,
)
from src.config import BrandService, default_catalog


def _request(active_brand="other", body=None):
    config = SimpleNamespace(brand_id=active_brand)

    async def json_body():
        return json.loads(json.dumps(body))

    state = SimpleNamespace(config=config, brands=BrandService(default_catalog()))
    return SimpleNamespace(app=SimpleNamespace(state=state), json=json_body)


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


def test_export_returns_manifest_as_attachment(dirs):
    user, _ = dirs
    _write_brand(user, "acme", "ACME")

    response = asyncio.run(export_brand(_request(), "acme"))

    assert b"name: ACME" in response.body
    assert 'filename="acme.yaml"' in response.headers["content-disposition"]


def test_export_unknown_brand_404(dirs):
    with pytest.raises(HTTPException) as err:
        asyncio.run(export_brand(_request(), "ghost"))
    assert err.value.status_code == 404


def test_manifest_returns_brand_config(dirs):
    user, _ = dirs
    _write_brand(user, "acme", "ACME")

    body = asyncio.run(brand_manifest(_request(), "acme"))

    assert body == {"brand": {"id": "acme", "name": "ACME"}}


def test_manifest_unknown_brand_404(dirs):
    with pytest.raises(HTTPException) as err:
        asyncio.run(brand_manifest(_request(), "ghost"))
    assert err.value.status_code == 404


def test_create_brand_returns_id(dirs):
    _, builtin = dirs
    _write_template(builtin)

    result = asyncio.run(create_brand(_request(
        body={"id": "acme", "name": "ACME", "tagline": "beep"})))

    assert result == {"id": "acme"}


def test_create_brand_maps_value_error_to_400(dirs):
    _, builtin = dirs
    _write_template(builtin)

    with pytest.raises(HTTPException) as err:
        asyncio.run(create_brand(_request(body={"id": "BAD ID", "name": "ACME"})))
    assert err.value.status_code == 400


def test_create_brand_missing_template_500(dirs):
    with pytest.raises(HTTPException) as err:
        asyncio.run(create_brand(_request(body={"id": "acme", "name": "ACME"})))
    assert err.value.status_code == 500


def test_delete_inactive_brand_no_restart(dirs):
    user, _ = dirs
    _write_brand(user, "acme", "ACME")

    result = asyncio.run(delete_brand(_request("other"), "acme"))

    assert result == {"id": "acme", "restarting": False}
    assert not (user / "acme.yaml").exists()


def test_delete_builtin_only_404(dirs):
    _, builtin = dirs
    _write_brand(builtin, "acme", "ACME")

    with pytest.raises(HTTPException) as err:
        asyncio.run(delete_brand(_request("other"), "acme"))
    assert err.value.status_code == 404
    assert (builtin / "acme.yaml").exists()


def test_delete_active_without_builtin_rejected(dirs):
    user, _ = dirs
    _write_brand(user, "acme", "ACME")

    with pytest.raises(HTTPException) as err:
        asyncio.run(delete_brand(_request("acme"), "acme"))
    assert err.value.status_code == 400
    assert (user / "acme.yaml").exists()


def test_delete_active_override_reverts_and_restarts(dirs, monkeypatch):
    user, builtin = dirs
    _write_brand(user, "acme", "EDITED")
    _write_brand(builtin, "acme", "ACME")

    restarts = []
    monkeypatch.setattr("src.api.routes.brands.schedule_restart", lambda: restarts.append(True))

    result = asyncio.run(delete_brand(_request("acme"), "acme"))

    assert result == {"id": "acme", "restarting": True}
    assert not (user / "acme.yaml").exists()
    assert (builtin / "acme.yaml").exists()
