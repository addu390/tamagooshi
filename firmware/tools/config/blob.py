"""Builds the runtime-config blob (usage: blob.py <brand-config.yaml> <out.bin>).

The TMG1 wire format must stay in lockstep with firmware/lib/core/config.cpp and
docs/js/config-builder.js.
"""

import json
import os
import struct
import sys

MAGIC = b"TMG1"
MAX_PAYLOAD = 0xFFFF

_GEN = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "generator")


def tz_minutes(value):
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


def _enabled(value):
    if value == "all" or value == ["all"]:
        return []
    return value or []


def _logo(data, base_dir):
    src = (data.get("brand") or {}).get("logo")
    if not src or not base_dir:
        return None
    if _GEN not in sys.path:
        sys.path.insert(0, _GEN)
    from engine import logo_mask, pack_mask
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
            "packs": _enabled(mascot.get("enabled")),
            "themes": _enabled(theme.get("enabled")),
            "typefaces": _enabled(typeface.get("enabled")),
        },
    }
    logo = _logo(data, base_dir)
    if logo:
        config["logo"] = logo
    return config


def encode(config):
    payload = json.dumps(config, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(f"config payload {len(payload)}B exceeds {MAX_PAYLOAD}B")
    return MAGIC + struct.pack("<H", len(payload)) + payload


def _load_yaml(path):
    try:
        import yaml
    except ImportError:
        raise SystemExit("PyYAML is required: pip install pyyaml")
    with open(path, "r", encoding="utf-8") as fh:
        return yaml.safe_load(fh) or {}


def main(argv):
    if len(argv) != 3:
        raise SystemExit("usage: blob.py <brand-config.yaml> <out.bin>")
    config = from_brand(_load_yaml(argv[1]), os.path.dirname(os.path.abspath(argv[1])))
    blob = encode(config)
    with open(argv[2], "wb") as fh:
        fh.write(blob)
    print(f"wrote {len(blob)}B config blob -> {argv[2]}")


if __name__ == "__main__":
    main(sys.argv)
