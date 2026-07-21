import json
import struct

from gen.features.mascots import MASCOTS
from gen.images import logo_mask, pack_mask
from gen.manifest import is_all, select_mascots, tz_minutes
from gen.ui.themes import derive

MAGIC = b"TMG1"
MAX_PAYLOAD = 0xFFFF


def _enabled(value):
    if value == "all" or value == ["all"]:
        return []
    return value or []


def _packs(mascot):
    if is_all(mascot.get("enabled") or []):
        return []
    ids, customs = select_mascots(mascot)
    return list(dict.fromkeys([MASCOTS[i]["cat"] for i in ids] + [m["cat"] for m in customs]))


def _custom_themes(theme):
    return [{"name": c["name"], "colors": derive(c["colors"])}
            for c in theme.get("custom") or []]


def _logo(data, base_dir):
    src = (data.get("brand") or {}).get("logo")
    if not src or not base_dir:
        return None
    w, h, rows = logo_mask(src, base_dir)
    return {"w": w, "h": h, "bits": pack_mask(w, h, rows)}


def from_brand(data, base_dir=None):
    brand = data.get("brand") or {}
    device = data.get("device") or {}
    theme = device.get("theme") or {}
    typeface = device.get("typeface") or {}
    mascot = device.get("mascot") or {}

    config = {
        "brand": {
            "name": brand.get("name", ""),
            "tagline": brand.get("tagline", ""),
            "website": brand.get("website", ""),
            "mascot": brand.get("mascot", ""),
        },
        "defaults": {
            "theme": theme.get("default", ""),
            "typeface": typeface.get("default", ""),
            "mascot": mascot.get("default", ""),
            "mood": mascot.get("mood", "happy"),
            "tz": tz_minutes(device.get("timezone")),
        },
        "enabled": {
            "games": _enabled((device.get("games") or {}).get("enabled")),
            "apps": _enabled((device.get("apps") or {}).get("enabled")),
            "packs": _packs(mascot),
            "themes": _enabled(theme.get("enabled")),
            "typefaces": _enabled(typeface.get("enabled")),
        },
    }
    customs = _custom_themes(theme)
    if customs:
        config["themes"] = customs
    logo = _logo(data, base_dir)
    if logo:
        config["logo"] = logo
    return config


def encode(config):
    payload = json.dumps(config, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(f"config payload {len(payload)}B exceeds {MAX_PAYLOAD}B")
    return MAGIC + struct.pack("<H", len(payload)) + payload
