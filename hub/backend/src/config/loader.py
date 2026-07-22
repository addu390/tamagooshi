from __future__ import annotations

import os

from ..model import AlertRule, MoodRule
from ..services.sources import parse_sources
from .models import HubConfig
from .settings import load_settings
from .source import BrandSource
from .wiring import default_catalog


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


def apply_scene(cfg: HubConfig, scenes: dict, scene: str) -> HubConfig:
    active = scenes.get(scene)
    if active is None:
        return cfg
    if "sources" in active:
        cfg.sources = parse_sources(active["sources"] or [])
    if "moods" in active:
        cfg.moods = [MoodRule.model_validate(m) for m in (active["moods"] or [])]
    if "alerts" in active:
        cfg.alerts = [AlertRule.model_validate(a) for a in (active["alerts"] or [])]
    return cfg


def active_brand() -> str:
    return os.environ.get("TAMA_BRAND") or load_settings().get("brand") or "gooshi"


def load_config(source: BrandSource | None = None) -> HubConfig:
    source = source or default_catalog()
    brand = active_brand()
    cfg = hub_config_from_manifest(source.manifest(brand),
                                   os.environ.get("TAMA_DEVICE_ID", "sim"))

    scene = os.environ.get("TAMA_SCENE")
    if scene:
        cfg = apply_scene(cfg, source.scenes(brand), scene)

    env_broker = os.environ.get("TAMA_BROKER")
    if env_broker:
        host, _, port = env_broker.partition(":")
        cfg.broker.host = host
        if port:
            cfg.broker.port = int(port)
    return cfg
