import os

from gen import registry
from gen.features.mascots import MASCOT_CATEGORIES, MASCOTS
from gen.network.transports import DEFAULT_PROTOCOL, LINKS, PROTOCOLS
from gen.ui.themes import derive as derive_theme


def manifest_candidates(brands_dir, brand_id):
    # Order shared with hub/backend/src/config/loader.py
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


def select_features(cfg, category):
    inc = cfg.get("enabled") or []
    if is_all(inc):
        return category.selectable()
    for item in inc:
        if item not in category.items:
            raise SystemExit(f"unknown {category.noun}: {item}")
    return inc


def select_options(cfg, category, custom=None):
    custom = custom or {}
    noun = category.noun
    default = cfg.get("default")
    enabled = cfg.get("enabled") or ([default] if default else [])
    if is_all(enabled):
        enabled = [*category.items, *custom]
    if not enabled:
        raise SystemExit(f"{noun}.enabled is empty and no {noun}.default set")

    out = []
    for name in enabled:
        spec = custom.get(name) or category.items.get(name)
        if spec is None:
            raise SystemExit(f"unknown {noun}: {name}")
        out.append((name, spec))
    if default not in [n for n, _ in out]:
        raise SystemExit(f"default {noun} '{default}' is not in {noun}.enabled")
    return out, default


def select_themes(cfg):
    custom = {c["name"]: {"roles": derive_theme(c["colors"])} for c in cfg.get("custom") or []}
    return select_options(cfg, registry.themes, custom)


def select_mascots(cfg):
    declared = [{"id": c["id"], "label": c["label"], "cat": c.get("category", "custom"),
                 "src": c["source"]} for c in cfg.get("custom") or []]
    custom_ids = {m["id"] for m in declared}
    custom_cats = {m["cat"] for m in declared}

    enabled = cfg.get("enabled") or []
    if is_all(enabled):
        enabled = [*MASCOT_CATEGORIES, *dict.fromkeys(m["cat"] for m in declared)]

    ids, customs = [], []
    for entry in enabled:
        if entry in MASCOT_CATEGORIES:
            ids += MASCOT_CATEGORIES[entry]
        elif entry in MASCOTS:
            ids.append(entry)
        elif entry in custom_cats:
            customs += [m for m in declared if m["cat"] == entry]
        elif entry in custom_ids:
            customs += [m for m in declared if m["id"] == entry]
        else:
            raise SystemExit(f"unknown mascot or category: {entry}")

    return list(dict.fromkeys(ids)), list({m["id"]: m for m in customs}.values())


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


# Same as hub/backend/src/config/loader.py _tz_minutes
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
