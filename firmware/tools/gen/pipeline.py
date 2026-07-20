import os

from gen import registry
from gen.emit.headers import (emit_boards, emit_brand, emit_features, emit_logo, emit_mascots,
                              emit_portal, emit_roles, emit_themes, emit_typefaces)
from gen.manifest import (load, parse_transports, resolve_manifest, select_features,
                          select_mascots, select_options, select_themes, tz_minutes)
from gen.network.transports import transport_macros


def generate(brand_id, brands_dir, out_dir, dev_name=""):
    path = resolve_manifest(brand_id, brands_dir)
    data = load(path)
    base_dir = os.path.dirname(path)

    device = data.get("device") or {}
    mascot = device.get("mascot") or {}
    ids, customs = select_mascots(mascot)
    default_mascot = mascot.get("default")
    every = [*ids, *[m["id"] for m in customs]]
    if not every:
        raise SystemExit("no mascots selected")
    if default_mascot not in every:
        raise SystemExit(f"default mascot '{default_mascot}' is not in the enabled mascots")

    themes, default_theme = select_themes(device.get("theme") or {})
    typefaces, default_typeface = select_options(device.get("typeface") or {}, registry.typefaces)
    games = select_features(device.get("games") or {}, registry.games)
    apps = select_features(device.get("apps") or {}, registry.apps)
    spec = parse_transports(device.get("transports"))
    transports = transport_macros(spec)
    buddy = bool((device.get("buddy") or {}).get("enabled", True)) and "ble" in spec
    tz_offset_min = tz_minutes(device.get("timezone"))
    default_mood = mascot.get("mood", "happy")

    emit_boards(out_dir)
    emit_roles(out_dir)
    emit_mascots(out_dir, ids, customs, base_dir)
    emit_themes(out_dir, themes)
    emit_typefaces(out_dir, typefaces)
    emit_features(out_dir, registry.apps, apps)
    emit_features(out_dir, registry.games, games)
    emit_portal(out_dir)
    logo_id = emit_logo(out_dir, data, base_dir, (data.get("brand") or {}).get("id", brand_id))
    emit_brand(out_dir, brand_id, data, default_mascot, default_theme, default_typeface,
               default_mood, tz_offset_min, games, apps, logo_id, dev_name, buddy)
    return transports
