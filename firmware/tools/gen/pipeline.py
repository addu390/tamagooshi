import os

from gen import registry
from gen.emit.headers import (
    emit_boards,
    emit_brand,
    emit_features,
    emit_hid_modes,
    emit_logo,
    emit_mascots,
    emit_persona,
    emit_portal,
    emit_roles,
    emit_themes,
    emit_typefaces,
)
from gen.manifest import (
    load,
    parse_transports,
    resolve_manifest,
    select_features,
    select_mascots,
    select_options,
    select_persona,
    select_themes,
    tz_minutes,
)
from gen.network.agents import agent_macros, parse_agent, supports
from gen.network.transports import transport_macros


def _persona_custom(persona):
    return {k: persona[k] for k in ("id", "label", "cat", "src")}


def _select_cast(device):
    mascot = device.get("mascot") or {}
    ids, customs = select_mascots(mascot)
    persona = select_persona(device)

    if persona and any(m["id"] == "persona" for m in customs):
        raise SystemExit("mascot id 'persona' is reserved")
    if persona and persona["mascot"]:
        customs = [*customs, _persona_custom(persona)]

    default = mascot.get("default")
    every = [*ids, *[m["id"] for m in customs]]
    if not every:
        raise SystemExit("no mascots selected")
    if default not in every:
        raise SystemExit(f"default mascot '{default}' is not in the enabled mascots")

    return ids, customs, persona, default, mascot.get("mood", "happy")


def _without_hid(ids, category, spec):
    if "ble" in spec:
        return ids
    return [i for i in ids if not category.items[i].get("hid")]


def _without_agent(ids, category, agent, board):
    if supports(agent, board):
        return ids
    return [i for i in ids if not category.items[i].get("agent")]


def _select_device(device, transports_override, agent, board):
    themes, default_theme = select_themes(device.get("theme") or {})
    typefaces, default_typeface = select_options(
        device.get("typeface") or {}, registry.typefaces)

    games = select_features(device.get("games") or {}, registry.games)
    apps = select_features(device.get("apps") or {}, registry.apps)
    spec = parse_transports(transports_override or device.get("transports"))
    games = _without_hid(games, registry.games, spec)
    apps = _without_hid(apps, registry.apps, spec)
    apps = _without_agent(apps, registry.apps, agent, board)

    tz_offset_min = tz_minutes(device.get("timezone"))

    return (themes, default_theme, typefaces, default_typeface,
            games, apps, spec, tz_offset_min)


def _hid_kinds(games, apps):
    used = ({registry.games.items[i].get("hid") for i in games}
            | {registry.apps.items[i].get("hid") for i in apps})
    return [k for k in registry.HID_KINDS if k in used]


def _emit(out_dir, brand_id, data, base_dir, ids, customs, persona,
          default_mascot, default_theme, default_typeface, default_mood,
          themes, typefaces, tz_offset_min, games, apps):
    emit_boards(out_dir)
    emit_roles(out_dir)
    emit_hid_modes(out_dir, _hid_kinds(games, apps))

    emit_mascots(out_dir, ids, customs, base_dir)
    emit_persona(out_dir, persona, base_dir)

    emit_themes(out_dir, themes)
    emit_typefaces(out_dir, typefaces)
    emit_features(out_dir, registry.apps, apps)
    emit_features(out_dir, registry.games, games)
    emit_portal(out_dir)

    logo_id = emit_logo(out_dir, data, base_dir, (data.get("brand") or {}).get("id", brand_id))
    emit_brand(out_dir, brand_id, data, default_mascot, default_theme, default_typeface,
               default_mood, tz_offset_min, games, apps, logo_id, persona)


def generate(brand_id, brands_dir, out_dir, transports_override=None, board=None):
    path = resolve_manifest(brand_id, brands_dir)
    data = load(path)
    base_dir = os.path.dirname(path)
    device = data.get("device") or {}

    agent = parse_agent(device.get("agent"))
    ids, customs, persona, default_mascot, default_mood = _select_cast(device)
    (themes, default_theme, typefaces, default_typeface,
     games, apps, spec, tz_offset_min) = _select_device(device, transports_override, agent, board)

    _emit(out_dir, brand_id, data, base_dir, ids, customs, persona,
          default_mascot, default_theme, default_typeface, default_mood,
          themes, typefaces, tz_offset_min, games, apps)

    links = sorted(set(transport_macros(spec)) | set(agent_macros(agent, board)))
    return links + registry.hid_macros(games, apps)
