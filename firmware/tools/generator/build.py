import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import engine
from apps import APPS
from boards import BOARDS, SCREEN_H, SCREEN_W, macro as board_macro
from games import GAMES
from mascots import MASCOT_CATEGORIES, MASCOTS
from themes import THEMES, derive as derive_theme
from typefaces import TYPEFACES

GAME_MACRO = {g: f"TAMA_GAME_{g.upper()}" for g in GAMES}
APP_MACRO = {a: f"TAMA_APP_{a.upper()}" for a in APPS}

LINKS = ("ble", "wifi")
LINK_MACRO = {"ble": "TAMA_ENABLE_BLE", "wifi": "TAMA_ENABLE_WIFI"}
PROTOCOLS = {"ble": ("gatt",), "wifi": ("mqtt",)}
DEFAULT_PROTOCOL = {"ble": "gatt", "wifi": "mqtt"}
PROTOCOL_MACRO = {"gatt": "TAMA_PROTO_GATT", "mqtt": "TAMA_PROTO_MQTT"}


def resolve_manifest(brand_id, brands_dir):
    folder = os.path.join(brands_dir, brand_id, "config.yaml")
    flat = os.path.join(brands_dir, brand_id + ".yaml")
    path = folder if os.path.exists(folder) else flat
    if not os.path.exists(path):
        raise SystemExit(f"brand '{brand_id}' not found (looked for {folder} and {flat})")
    return path


def _load(path):
    try:
        import yaml
    except ImportError:
        raise SystemExit("PyYAML is required to generate brands: pip install pyyaml")
    with open(path, "r", encoding="utf-8") as fh:
        return yaml.safe_load(fh) or {}


def _cstr(s):
    return '"' + str(s).replace("\\", "\\\\").replace('"', '\\"') + '"'


def _hostname(url):
    if not url:
        return ""
    s = str(url).strip()
    for scheme in ("https://", "http://"):
        if s.lower().startswith(scheme):
            s = s[len(scheme):]
    if s.lower().startswith("www."):
        s = s[4:]
    return s.split("/")[0].rstrip("/")


def _is_all(value):
    return value == "all" or value == ["all"]


def _playable_games():
    return [g for g, meta in GAMES.items() if not meta.get("soon")]


def _select_mascots(cfg):
    enabled = cfg.get("enabled") or []
    if _is_all(enabled):
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


def _select_themes(cfg):
    custom = {c["name"]: derive_theme(c["colors"]) for c in cfg.get("custom") or []}
    default = cfg.get("default")
    enabled = cfg.get("enabled") or ([default] if default else [])
    if _is_all(enabled):
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


def _select_typefaces(cfg):
    default = cfg.get("default")
    enabled = cfg.get("enabled") or ([default] if default else [])
    if _is_all(enabled):
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


def _select_games(cfg):
    inc = cfg.get("enabled") or []
    if _is_all(inc):
        return _playable_games()
    for g in inc:
        if g not in GAMES:
            raise SystemExit(f"unknown game: {g}")
    return inc


def _select_apps(cfg):
    inc = cfg.get("enabled") or []
    if _is_all(inc):
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


def transport_macros(spec):
    hub = spec.get("wifi") or ("gatt" if "ble" in spec else None)
    if hub is None:
        raise SystemExit("transports need a bearer that can carry the hub")
    macros = {LINK_MACRO[link] for link in spec}
    macros.add(PROTOCOL_MACRO[hub])
    return sorted(macros)


def _tz_minutes(value):
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


def _write(path, text):
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as fh:
            if fh.read() == text:
                return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)


def _emit_mascots(out_dir, ids, customs, base_dir):
    for mid in ids:
        _write(os.path.join(out_dir, "mascots", f"{mid}.h"),
               engine.sprite_header(MASCOTS[mid], base_dir))
    for m in customs:
        _write(os.path.join(out_dir, "mascots", f"{m['id']}.h"),
               engine.sprite_header(m, base_dir))

    every = [*ids, *[m["id"] for m in customs]]
    lines = ['#pragma once', '', '#include "registry.h"', '']
    lines += [f'#include "mascots/{i}.h"' for i in every]
    lines += ['', 'namespace tama::characters {', '',
              'inline void registerGenerated(CharacterRegistry& registry) {']
    lines += [f'  registry.add({i}());' for i in every]
    lines += ['}', '', '}  // namespace tama::characters', '']
    _write(os.path.join(out_dir, "mascots.gen.h"), "\n".join(lines))


def _emit_themes(out_dir, themes):
    rows = []
    for name, roles in themes:
        vals = ", ".join("0x%04X" % v for v in roles)
        rows.append(f'    {{{_cstr(name)}, {vals}}},')
    text = "\n".join(['#pragma once', '', 'const Theme kThemes[] = {', *rows, '};', ''])
    _write(os.path.join(out_dir, "themes.gen.h"), text)


def _emit_typefaces(out_dir, typefaces):
    includes = []
    for _, spec in typefaces:
        inc = spec.get("include")
        if inc and inc not in includes:
            includes.append(inc)
    rows = []
    for name, spec in typefaces:
        micro, body, title = spec["roles"]
        rows.append(f'    {{{_cstr(name)}, {micro}, {body}, {title}}},')
    lines = ['#pragma once', '']
    lines += [f'#include {inc}' for inc in includes]
    if includes:
        lines.append('')
    lines += ['const Typeface kTypefaces[] = {', *rows, '};', '']
    _write(os.path.join(out_dir, "typefaces.gen.h"), "\n".join(lines))


def _emit_logo(out_dir, data, base_dir, brand_id):
    src = (data.get("brand") or {}).get("logo")
    if not src:
        _write(os.path.join(out_dir, "logo.gen.h"), "#pragma once\n#define TAMA_HAS_LOGO 0\n")
        return ""

    w, h, mask = engine.logo_mask(src, base_dir)
    flat = ",".join(str(mask[y][x]) for y in range(h) for x in range(w))
    lines = [
        '#pragma once', '', '#define TAMA_HAS_LOGO 1', '',
        '#include <algorithm>', '#include <cstdint>', '', '#include "gfx.h"', '',
        'namespace tama::logos {', '',
        f'inline constexpr const char* kBrandLogoId = {_cstr(brand_id)};',
        f'inline constexpr int kLogoW = {w};', f'inline constexpr int kLogoH = {h};',
        f'inline constexpr uint8_t kLogoMask[kLogoW * kLogoH] = {{ {flat} }};', '',
        'inline void drawBrandLogo(Gfx& g, int cx, int cy, int size, uint16_t col) {',
        '  const int h = std::max(1, size);',
        '  const int w = std::max(1, kLogoW * h / kLogoH);',
        '  const int ox = cx - w / 2;',
        '  const int oy = cy - h / 2;',
        '  for (int dy = 0; dy < h; ++dy) {',
        '    const int sy = dy * kLogoH / h;',
        '    for (int dx = 0; dx < w; ++dx) {',
        '      const int sx = dx * kLogoW / w;',
        '      if (kLogoMask[sy * kLogoW + sx]) g.c().drawPixel(ox + dx, oy + dy, col);',
        '    }',
        '  }',
        '}', '', '}  // namespace tama::logos', '',
    ]
    _write(os.path.join(out_dir, "logo.gen.h"), "\n".join(lines))
    return brand_id


def _minify_html(text):
    lines = (line.strip() for line in text.splitlines())
    return "".join(line for line in lines if line)


def _emit_boards(out_dir):
    flags = ("buzzer", "speaker", "mic", "imu", "joystick", "haptics", "ir", "wearable")
    lines = ['#pragma once', '', '#include "model.h"', '', 'namespace tama::board {', '']
    for i, (bid, b) in enumerate(BOARDS.items()):
        guard = "#if" if i == 0 else "#elif"
        c = b["caps"]
        lines += [
            f'{guard} defined({board_macro(bid)})',
            f'#define TAMA_M5_FALLBACK_BOARD {b["m5_board"]}',
            f'inline constexpr int kRedLedPin = {b["led_pin"]};',
            'inline DeviceCapabilities capabilities() {',
            '  DeviceCapabilities caps;',
            f'  caps.model = {_cstr(bid)};',
            f'  caps.screenW = {SCREEN_W};',
            f'  caps.screenH = {SCREEN_H};',
            f'  caps.buttons = {c["buttons"]};',
            f'  caps.led = {_cstr(c["led"])};',
        ]
        lines += [f'  caps.{f} = {"true" if c[f] else "false"};' for f in flags]
        lines += ['  return caps;', '}']
    lines += [
        '#else',
        '#error "No TAMA_BOARD_* defined; set one in the PlatformIO env build_flags"',
        '#endif', '', '}  // namespace tama::board', '',
    ]
    _write(os.path.join(out_dir, "board.gen.h"), "\n".join(lines))


def _emit_portal(out_dir):
    src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "portal.html")
    with open(src, "r", encoding="utf-8") as fh:
        head, setup, saved = (_minify_html(p) for p in fh.read().split("<!--PART-->"))
    lines = [
        '#pragma once', '',
        'namespace tama::portal {', '',
        f'inline constexpr const char kHead[] = R"HTML({head})HTML";',
        f'inline constexpr const char kSetup[] = R"HTML({setup})HTML";',
        f'inline constexpr const char kSaved[] = R"HTML({saved})HTML";', '',
        '}  // namespace tama::portal', '',
    ]
    _write(os.path.join(out_dir, "portal.gen.h"), "\n".join(lines))


def _emit_brand(out_dir, brand_id, data, default_mascot, default_theme, default_typeface,
                default_mood, tz_offset_min, games, apps, logo_id, dev_name):
    ident = data.get("brand") or {}
    lines = ['#pragma once', '',
             f'#define TAMA_BRAND_ID {_cstr(ident.get("id", brand_id))}',
             f'#define TAMA_PRODUCT_NAME {_cstr(ident.get("name", "TAMAGOOSHI"))}',
             f'#define TAMA_TAGLINE {_cstr(ident.get("tagline", ""))}',
             f'#define TAMA_WEBSITE {_cstr(_hostname(ident.get("website", "")))}',
             f'#define TAMA_MASCOT_NAME {_cstr(ident.get("mascot", ""))}',
             f'#define TAMA_LOGO_ID {_cstr(logo_id)}',
             f'#define TAMA_DEV_NAME {_cstr(dev_name)}',
             f'#define TAMA_DEFAULT_MASCOT {_cstr(default_mascot)}',
             f'#define TAMA_DEFAULT_THEME {_cstr(default_theme)}',
             f'#define TAMA_DEFAULT_TYPEFACE {_cstr(default_typeface)}',
             f'#define TAMA_DEFAULT_MOOD {_cstr(default_mood)}',
             f'#define TAMA_TZ_OFFSET_MIN {int(tz_offset_min)}', '']
    lines += [f'#define {GAME_MACRO[g]} 1' for g in games]
    lines += [f'#define {APP_MACRO[a]} 1' for a in apps]
    lines += ['',
              '#include "model.h"', '#include "theme.h"', '#include "typeface.h"', '',
              'namespace tama::brand {', '',
              'inline void apply(DeviceState& state) {',
              '  state.branding.name = TAMA_PRODUCT_NAME;',
              '  state.branding.tagline = TAMA_TAGLINE;',
              '  state.branding.website = TAMA_WEBSITE;',
              '  state.branding.logo_id = TAMA_LOGO_ID;',
              '  state.branding.mascot_name = TAMA_MASCOT_NAME;',
              '  state.branding.dev_name = TAMA_DEV_NAME;',
              '  state.character_id = TAMA_DEFAULT_MASCOT;',
              '  state.mood = moodFromString(TAMA_DEFAULT_MOOD);',
              '  state.tz_offset_min = TAMA_TZ_OFFSET_MIN;',
              '  theme::setThemeByName(TAMA_DEFAULT_THEME);',
              '  typeface::setTypefaceByName(TAMA_DEFAULT_TYPEFACE);',
              '}', '',
              '}  // namespace tama::brand', '']
    _write(os.path.join(out_dir, "brand.gen.h"), "\n".join(lines))


def generate(brand_id, brands_dir, out_dir, dev_name=""):
    path = resolve_manifest(brand_id, brands_dir)
    data = _load(path)
    base_dir = os.path.dirname(path)

    device = data.get("device") or {}
    mascot = device.get("mascot") or {}
    ids, customs = _select_mascots(mascot)
    default_mascot = mascot.get("default")
    every = [*ids, *[m["id"] for m in customs]]
    if not every:
        raise SystemExit("no mascots selected")
    if default_mascot not in every:
        raise SystemExit(f"default mascot '{default_mascot}' is not in the enabled mascots")

    themes, default_theme = _select_themes(device.get("theme") or {})
    typefaces, default_typeface = _select_typefaces(device.get("typeface") or {})
    games = _select_games(device.get("games") or {})
    apps = _select_apps(device.get("apps") or {})
    transports = transport_macros(parse_transports(device.get("transports")))
    tz_offset_min = _tz_minutes(device.get("timezone"))
    default_mood = mascot.get("mood", "happy")

    _emit_boards(out_dir)
    _emit_mascots(out_dir, ids, customs, base_dir)
    _emit_themes(out_dir, themes)
    _emit_typefaces(out_dir, typefaces)
    _emit_portal(out_dir)
    logo_id = _emit_logo(out_dir, data, base_dir, (data.get("brand") or {}).get("id", brand_id))
    _emit_brand(out_dir, brand_id, data, default_mascot, default_theme, default_typeface,
                default_mood, tz_offset_min, games, apps, logo_id, dev_name)
    return transports


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    firmware = os.path.abspath(os.path.join(here, "..", ".."))
    repo = os.path.abspath(os.path.join(firmware, ".."))
    brand = argv[1] if len(argv) > 1 else os.environ.get("TAMA_BRAND", "gooshi")
    out = argv[2] if len(argv) > 2 else os.path.join(firmware, ".gen", "current")
    generate(brand, os.path.join(repo, "brands"), out, os.environ.get("TAMA_DEV", ""))
    print(f"generated {brand} -> {out}")


if __name__ == "__main__":
    main(sys.argv)
