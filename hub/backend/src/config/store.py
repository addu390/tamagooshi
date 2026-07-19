from __future__ import annotations

import os
import re
import shutil

import yaml

from .loader import brand_dirs, hub_config_from_manifest, resolve_brand
from .settings import user_brands_dir

IDENTITY_FIELDS = ("name", "tagline", "website", "mascot")
BRAND_ID = re.compile(r"[a-z0-9][a-z0-9-]*")
TEMPLATE_BRAND = "template"


def _user_path(brand_id: str) -> str:
    folder = os.path.join(user_brands_dir(), brand_id, "config.yaml")
    if os.path.exists(folder):
        return folder
    return os.path.join(user_brands_dir(), f"{brand_id}.yaml")


def read_manifest(brand_id: str) -> dict:
    with open(resolve_brand(brand_id), "r", encoding="utf-8") as fh:
        return yaml.safe_load(fh) or {}


def write_manifest(brand_id: str, data: dict) -> str:
    path = _user_path(brand_id)
    os.makedirs(os.path.dirname(path), exist_ok=True)

    with open(path, "w", encoding="utf-8") as fh:
        yaml.safe_dump(data, fh, sort_keys=False, allow_unicode=True)
    return path


def mutate_manifest(brand_id: str, mutate) -> dict:
    data = read_manifest(brand_id)
    mutate(data)

    hub_config_from_manifest(data)
    write_manifest(brand_id, data)
    return data


def _manifest_ids() -> dict[str, str]:
    found: dict[str, str] = {}

    for source, brands in zip(("user", "builtin"), brand_dirs()):
        if not os.path.isdir(brands):
            continue

        for entry in sorted(os.listdir(brands)):
            path = os.path.join(brands, entry)

            brand_id = None
            if os.path.isdir(path) and os.path.isfile(os.path.join(path, "config.yaml")):
                brand_id = entry
            elif entry.endswith((".yaml", ".yml")):
                brand_id = entry.rsplit(".", 1)[0]

            if brand_id and brand_id not in found:
                found[brand_id] = source

    return found


def user_copy(brand_id: str) -> str | None:
    base = user_brands_dir()

    folder = os.path.join(base, brand_id)
    if os.path.isfile(os.path.join(folder, "config.yaml")):
        return folder

    flat = os.path.join(base, f"{brand_id}.yaml")
    return flat if os.path.isfile(flat) else None


def builtin_exists(brand_id: str) -> bool:
    builtin = brand_dirs()[1]
    return (os.path.isfile(os.path.join(builtin, brand_id, "config.yaml"))
            or os.path.isfile(os.path.join(builtin, f"{brand_id}.yaml")))


def list_brands() -> list[dict]:
    out = []

    for brand_id, source in _manifest_ids().items():
        ident = read_manifest(brand_id).get("brand") or {}
        out.append({
            "id": brand_id,
            "name": ident.get("name", brand_id),
            "tagline": ident.get("tagline"),
            "source": source,
            "overrides_builtin": source == "user" and builtin_exists(brand_id),
        })

    return out


def import_manifest(data: dict) -> str:
    hub_config_from_manifest(data)

    brand_id = (data.get("brand") or {}).get("id")
    if not brand_id:
        raise ValueError("brand.id missing")

    write_manifest(brand_id, data)
    return brand_id


def create_brand(brand_id: str, name: str, tagline: str = "") -> str:
    if not BRAND_ID.fullmatch(brand_id):
        raise ValueError("id must be lowercase letters, numbers and dashes")
    if not name:
        raise ValueError("name is required")
    if brand_id in _manifest_ids():
        raise ValueError(f"brand '{brand_id}' already exists")

    data = read_manifest(TEMPLATE_BRAND)
    data["brand"] = {**(data.get("brand") or {}), "id": brand_id, "name": name,
                     "tagline": tagline}
    hub_config_from_manifest(data)

    write_manifest(brand_id, data)
    return brand_id


def delete_user_brand(brand_id: str) -> None:
    target = user_copy(brand_id)
    if target is None:
        raise FileNotFoundError(f"no user copy of brand '{brand_id}'")

    if os.path.isdir(target):
        shutil.rmtree(target)
    else:
        os.remove(target)


def _mutate_sources(brand_id: str, mutate) -> dict:
    def apply(data: dict) -> None:
        mutate(data.setdefault("hub", {}).setdefault("sources", []))

    return mutate_manifest(brand_id, apply)


def set_source_enabled(brand_id: str, index: int, enabled: bool) -> None:
    def mutate(sources: list) -> None:
        sources[index]["enabled"] = enabled

    _mutate_sources(brand_id, mutate)


def add_source(brand_id: str, source: dict) -> None:
    _mutate_sources(brand_id, lambda sources: sources.append(source))


def update_source(brand_id: str, index: int, source: dict) -> None:
    def mutate(sources: list) -> None:
        sources[index] = source

    _mutate_sources(brand_id, mutate)


def remove_source(brand_id: str, index: int) -> None:
    _mutate_sources(brand_id, lambda sources: sources.pop(index))


def update_identity(brand_id: str, identity: dict) -> dict:
    def apply(data: dict) -> None:
        brand = data.setdefault("brand", {})
        for field in IDENTITY_FIELDS:
            if field not in identity:
                continue
            value = identity[field]
            if value in (None, ""):
                brand.pop(field, None)
            else:
                brand[field] = value

    return mutate_manifest(brand_id, apply)


def update_device(brand_id: str, device: dict) -> dict:
    def apply(data: dict) -> None:
        data["device"] = device

    return mutate_manifest(brand_id, apply)


def update_rules(brand_id: str, moods: list | None = None, alerts: list | None = None) -> dict:
    def apply(data: dict) -> None:
        hub = data.setdefault("hub", {})
        if moods is not None:
            hub["moods"] = moods
        if alerts is not None:
            hub["alerts"] = alerts

    return mutate_manifest(brand_id, apply)


def update_agent(brand_id: str, default: str, enabled: list) -> dict:
    def apply(data: dict) -> None:
        data.setdefault("hub", {})["agent"] = {"default": default, "enabled": enabled}

    return mutate_manifest(brand_id, apply)
