import os

from gen.features.apps import APPS
from gen.features.games import GAMES
from gen.features.mascots import MASCOT_CATEGORIES, MASCOTS
from gen.network.transports import DEFAULT_PROTOCOL, LINKS, PROTOCOLS
from gen.ui.themes import THEMES, derive as derive_theme
from gen.ui.typefaces import TYPEFACES


def manifest_candidates(brands_dir, brand_id):
    # Order shared with hub/src/config/loader.py
    return [os.path.join(brands_dir, brand_id, "config.yaml"),
            os.path.join(brands_dir, brand_id + ".yaml")]


def resolve_manifest(brand_id, brands_dir):
    candidates = manifest_candidates(brands_dir, brand_id)
    for path in candidates:
        if os.path.exists(path):
            return path
    raise SystemExit(f"brand '{brand_id}' not found (looked for {' and '.join(candidates)})")


def load(path):
    try:
        import yaml
    except ImportError:
        raise SystemExit("PyYAML is required to generate brands: pip install pyyaml")
    with open(path, "r", encoding="utf-8") as fh:
        return yaml.safe_load(fh) or {}


def hostname(url):
    if not url:
        return ""
    s = str(url).strip()
    for scheme in ("https://", "http://"):
        if s.lower().startswith(scheme):
            s = s[len(scheme):]
    if s.lower().startswith("www."):
        s = s[4:]
    return s.split("/")[0].rstrip("/")


def is_all(value):
    return value == "all" or value == ["all"]


def playable_games():
    return [g for g, meta in GAMES.items() if not meta.get("soon")]


def select_mascots(cfg):
    enabled = cfg.get("enabled") or []
    if is_all(enabled):
        enabled = list(MASCOT_CATEGORIES)
    ids = []
    for entry in enabled:
        if entry in MASCOT_CATEGORIES:
            ids += MASCOT_CATEGORIES[entry]
        elif entry in MASCOTS:
            ids.append(entry)
        else:
            raise SystemExit(f"unknown mascot or category: {entry}")

    seen, ordered = set(), []
    for mid in ids:
        if mid not in seen:
            seen.add(mid)
            ordered.append(mid)

    customs = []
    for c in cfg.get("custom") or []:
        customs.append({"id": c["id"], "label": c["label"], "cat": c.get("category", "custom"),
                        "src": c["source"]})
    return ordered, customs


def select_themes(cfg):
    custom = {c["name"]: derive_theme(c["colors"]) for c in cfg.get("custom") or []}
    default = cfg.get("default")
    enabled = cfg.get("enabled") or ([default] if default else [])
    if is_all(enabled):
        enabled = list(THEMES) + list(custom)
    if not enabled:
        raise SystemExit("theme.enabled is empty and no theme.default set")

    out = []
    for name in enabled:
        if name in custom:
            out.append((name, custom[name]))
        elif name in THEMES:
            out.append((name, THEMES[name]))
        else:
            raise SystemExit(f"unknown theme: {name}")
    if default not in [n for n, _ in out]:
        raise SystemExit(f"default theme '{default}' is not in theme.enabled")
    return out, default


def select_typefaces(cfg):
    default = cfg.get("default")
    enabled = cfg.get("enabled") or ([default] if default else [])
    if is_all(enabled):
        enabled = list(TYPEFACES)
    if not enabled:
        raise SystemExit("typeface.enabled is empty and no typeface.default set")

    out = []
    for name in enabled:
        if name in TYPEFACES:
            out.append((name, TYPEFACES[name]))
        else:
            raise SystemExit(f"unknown typeface: {name}")
    if default not in [n for n, _ in out]:
        raise SystemExit(f"default typeface '{default}' is not in typeface.enabled")
    return out, default


def select_games(cfg):
    inc = cfg.get("enabled") or []
    if is_all(inc):
        return playable_games()
    for g in inc:
        if g not in GAMES:
            raise SystemExit(f"unknown game: {g}")
    return inc


def select_apps(cfg):
    inc = cfg.get("enabled") or []
    if is_all(inc):
        return list(APPS)
    for a in inc:
        if a not in APPS:
            raise SystemExit(f"unknown app: {a}")
    return inc


def parse_transports(value):
    if value is None:
        items = [("ble", None)]
    elif isinstance(value, str):
        items = [tok.strip().partition(":")[::2] for tok in value.split(",") if tok.strip()]
    elif isinstance(value, dict):
        items = list(value.items())
    else:
        items = [(t.partition(":")[0], t.partition(":")[2]) if isinstance(t, str) else
                 (t.get("link"), t.get("protocol")) for t in value]

    spec = {}
    for link, proto in items:
        if link not in LINKS:
            raise SystemExit(f"unknown transport link: {link}")
        proto = proto or DEFAULT_PROTOCOL[link]
        if proto not in PROTOCOLS[link]:
            raise SystemExit(f"link '{link}' does not support protocol '{proto}'")
        spec[link] = proto
    if not spec:
        raise SystemExit("device.transports is empty")
    return spec


# Same as hub/src/config/loader.py _tz_minutes
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
