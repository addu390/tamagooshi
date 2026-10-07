from __future__ import annotations

import os

from ..model import AlertRule, MoodRule
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
    theme = device.get("theme") or {}
    mascot = device.get("mascot") or {}

    for rule in device.get("moods") or []:
        MoodRule.model_validate(rule)
    for rule in device.get("alerts") or []:
        AlertRule.model_validate(rule)

    return HubConfig.model_validate({
        "device_id": device_id,
        "brand_id": brand.get("id", "gooshi"),
        "brand": {
            "name": brand.get("name", "TAMAGOOSHI"),
            "tagline": brand.get("tagline"),
            "logo_id": brand.get("id"),
            "theme": theme.get("default"),
            "mascot": mascot.get("default"),
            "carousel_secs": device.get("carousel_secs"),
            "tz_offset": _tz_minutes(device.get("timezone")),
        },
    })


def active_brand() -> str:
    return os.environ.get("TAMA_BRAND") or load_settings().get("brand") or "gooshi"


def load_config(source: BrandSource | None = None) -> HubConfig:
    source = source or default_catalog()
    brand = active_brand()
    return hub_config_from_manifest(source.manifest(brand),
                                    os.environ.get("TAMA_DEVICE_ID", "sim"))
