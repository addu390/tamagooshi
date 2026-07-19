from __future__ import annotations

import os
import sys

import yaml

from ..services.sources import parse_sources
from .scenes import apply_scene
from .models import HubConfig
from .settings import load_settings, user_brands_dir


def _builtin_brands_dir() -> str:
    override = os.environ.get("TAMA_BRANDS_DIR")
    if override:
        return override
    bundled = os.path.join(getattr(sys, "_MEIPASS", ""), "brands")
    if bundled != "brands" and os.path.isdir(bundled):
        return bundled
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(here, "..", "..", "..", "brands"))


def brand_dirs() -> list[str]:
    return [user_brands_dir(), _builtin_brands_dir()]


def manifest_candidates(brands_dir: str, brand_id: str) -> list[str]:
    # Order shared with firmware/tools/gen/manifest.py
    return [os.path.join(brands_dir, brand_id, "config.yaml"),
            os.path.join(brands_dir, brand_id + ".yaml")]


def resolve_brand(brand_id: str) -> str:
    for brands in brand_dirs():
        for candidate in manifest_candidates(brands, brand_id):
            if os.path.exists(candidate):
                return candidate
    raise FileNotFoundError(f"brand '{brand_id}' not found under {brand_dirs()}")


# Same as firmware/tools/gen/manifest.py tz_minutes
def _tz_minutes(value) -> int:
    if value is None:
        return 0
    if isinstance(value, (int, float)):
        return int(value)

    s = str(value).strip()
    if not s:
        return 0

    sign = 1
    if s[0] in "+-":
        sign = -1 if s[0] == "-" else 1
        s = s[1:]

    hh, _, mm = s.partition(":")
    return sign * (int(hh) * 60 + (int(mm) if mm else 0))


def hub_config_from_manifest(data: dict, device_id: str = "sim") -> HubConfig:
    brand = data.get("brand") or {}
    device = data.get("device") or {}
    hub = data.get("hub") or {}
    theme = device.get("theme") or {}
    mascot = device.get("mascot") or {}

    agent = hub.get("agent") or {}
    cfg = HubConfig.model_validate({
        "device_id": device_id,
        "brand_id": brand.get("id", "gooshi"),
        "agent": {
            "default": agent.get("default", "cursor"),
            "enabled": agent.get("enabled") or [],
        },
        "brand": {
            "name": brand.get("name", "TAMAGOOSHI"),
            "tagline": brand.get("tagline"),
            "logo_id": brand.get("id"),
            "theme": theme.get("default"),
            "mascot": mascot.get("default"),
            "carousel_secs": device.get("carousel_secs"),
            "tz_offset": _tz_minutes(device.get("timezone")),
        },
        "default_mood": mascot.get("mood", "happy"),
        "moods": hub.get("moods") or [],
        "alerts": hub.get("alerts") or [],
    })
    cfg.sources = parse_sources(hub.get("sources") or [])
    return cfg


def load_config() -> HubConfig:
    brand = os.environ.get("TAMA_BRAND") or load_settings().get("brand") or "gooshi"
    manifest = resolve_brand(brand)
    with open(manifest, "r", encoding="utf-8") as fh:
        data = yaml.safe_load(fh) or {}

    cfg = hub_config_from_manifest(data, os.environ.get("TAMA_DEVICE_ID", "sim"))

    scene = os.environ.get("TAMA_SCENE") or None
    if scene:
        cfg = apply_scene(cfg, manifest, scene)

    env_broker = os.environ.get("TAMA_BROKER")
    if env_broker:
        host, _, port = env_broker.partition(":")
        cfg.broker.host = host
        if port:
            cfg.broker.port = int(port)
    return cfg
