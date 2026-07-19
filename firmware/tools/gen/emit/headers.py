import os

from gen.emit.sprites import sprite_header
from gen.features.apps import APPS
from gen.features.games import GAMES
from gen.features.mascots import MASCOTS
from gen.images import logo_mask
from gen.manifest import hostname
from gen.platform.boards import BOARDS, SCREEN_H, SCREEN_W, macro as board_macro

GAME_MACRO = {g: f"TAMA_GAME_{g.upper()}" for g in GAMES}
APP_MACRO = {a: f"TAMA_APP_{a.upper()}" for a in APPS}

PORTAL = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                      "network", "portal.html")


def cstr(s):
    return '"' + str(s).replace("\\", "\\\\").replace('"', '\\"') + '"'


def write(path, text):
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as fh:
            if fh.read() == text:
                return
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text)


def emit_mascots(out_dir, ids, customs, base_dir):
    for mid in ids:
        write(os.path.join(out_dir, "mascots", f"{mid}.h"),
              sprite_header(MASCOTS[mid], base_dir))
    for m in customs:
        write(os.path.join(out_dir, "mascots", f"{m['id']}.h"),
              sprite_header(m, base_dir))

    every = [*ids, *[m["id"] for m in customs]]
    lines = ['#pragma once', '', '#include "mascots/registry.h"', '']
    lines += [f'#include "mascots/{i}.h"' for i in every]
    lines += ['', 'namespace tama::characters {', '',
              'inline void registerGenerated(CharacterRegistry& registry) {']
    lines += [f'  registry.add({i}());' for i in every]
    lines += ['}', '', '}  // namespace tama::characters', '']
    write(os.path.join(out_dir, "mascots.gen.h"), "\n".join(lines))


def emit_themes(out_dir, themes):
    rows = []
    for name, roles in themes:
        vals = ", ".join("0x%04X" % v for v in roles)
        rows.append(f'    {{{cstr(name)}, {vals}}},')
    text = "\n".join(['#pragma once', '', 'const Theme kThemes[] = {', *rows, '};', ''])
    write(os.path.join(out_dir, "themes.gen.h"), text)


def emit_typefaces(out_dir, typefaces):
    includes = []
    for _, spec in typefaces:
        inc = spec.get("include")
        if inc and inc not in includes:
            includes.append(inc)
    rows = []
    for name, spec in typefaces:
        micro, body, title, display = spec["roles"]
        rows.append(f'    {{{cstr(name)}, {micro}, {body}, {title}, {display}}},')
    lines = ['#pragma once', '']
    lines += [f'#include {inc}' for inc in includes]
    if includes:
        lines.append('')
    lines += ['const Typeface kTypefaces[] = {', *rows, '};', '']
    write(os.path.join(out_dir, "typefaces.gen.h"), "\n".join(lines))


def emit_logo(out_dir, data, base_dir, brand_id):
    src = (data.get("brand") or {}).get("logo")
    if not src:
        write(os.path.join(out_dir, "logo.gen.h"), "#pragma once\n#define TAMA_HAS_LOGO 0\n")
        return ""

    w, h, mask = logo_mask(src, base_dir)
    flat = ",".join(str(mask[y][x]) for y in range(h) for x in range(w))

    lines = [
        '#pragma once', '', '#define TAMA_HAS_LOGO 1', '',
        '#include <algorithm>', '#include <cstdint>', '', '#include "gfx.h"', '',
        'namespace tama::logos {', '',
        f'inline constexpr const char* kBrandLogoId = {cstr(brand_id)};',
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

    write(os.path.join(out_dir, "logo.gen.h"), "\n".join(lines))
    return brand_id


def _minify(text):
    lines = (line.strip() for line in text.splitlines())
    return "".join(line for line in lines if line)


def emit_boards(out_dir):
    flags = ("buzzer", "speaker", "mic", "imu", "joystick", "haptics", "ir", "wearable",
             "psram")
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
            f'  caps.model = {cstr(bid)};',
            f'  caps.screenW = {SCREEN_W};',
            f'  caps.screenH = {SCREEN_H};',
            f'  caps.buttons = {c["buttons"]};',
            f'  caps.led = {cstr(c["led"])};',
        ]
        lines += [f'  caps.{f} = {"true" if c[f] else "false"};' for f in flags]
        lines += ['  return caps;', '}']

    lines += [
        '#else',
        '#error "No TAMA_BOARD_* defined; set one in the PlatformIO env build_flags"',
        '#endif', '', '}  // namespace tama::board', '',
    ]

    write(os.path.join(out_dir, "board.gen.h"), "\n".join(lines))


def emit_portal(out_dir):
    with open(PORTAL, "r", encoding="utf-8") as fh:
        head, setup, saved = (_minify(p) for p in fh.read().split("<!--PART-->"))

    lines = [
        '#pragma once', '',
        'namespace tama::portal {', '',
        f'inline constexpr const char kHead[] = R"HTML({head})HTML";',
        f'inline constexpr const char kSetup[] = R"HTML({setup})HTML";',
        f'inline constexpr const char kSaved[] = R"HTML({saved})HTML";', '',
        '}  // namespace tama::portal', '',
    ]
    write(os.path.join(out_dir, "portal.gen.h"), "\n".join(lines))


def emit_brand(out_dir, brand_id, data, default_mascot, default_theme, default_typeface,
               default_mood, tz_offset_min, games, apps, logo_id, dev_name, buddy):
    ident = data.get("brand") or {}
    agent = (data.get("hub") or {}).get("agent") or {}
    agents = agent.get("enabled") or []
    agent_default = agent.get("default") or (agents[0] if agents else "")

    lines = ['#pragma once', '',
             f'#define TAMA_BRAND_ID {cstr(ident.get("id", brand_id))}',
             f'#define TAMA_PRODUCT_NAME {cstr(ident.get("name", "TAMAGOOSHI"))}',
             f'#define TAMA_TAGLINE {cstr(ident.get("tagline", ""))}',
             f'#define TAMA_WEBSITE {cstr(hostname(ident.get("website", "")))}',
             f'#define TAMA_MASCOT_NAME {cstr(ident.get("mascot", ""))}',
             f'#define TAMA_LOGO_ID {cstr(logo_id)}',
             f'#define TAMA_DEV_NAME {cstr(dev_name)}',
             f'#define TAMA_DEFAULT_MASCOT {cstr(default_mascot)}',
             f'#define TAMA_DEFAULT_THEME {cstr(default_theme)}',
             f'#define TAMA_DEFAULT_TYPEFACE {cstr(default_typeface)}',
             f'#define TAMA_DEFAULT_MOOD {cstr(default_mood)}',
             f'#define TAMA_TZ_OFFSET_MIN {int(tz_offset_min)}',
             f'#define TAMA_HUB_AGENTS {cstr(",".join(agents))}',
             f'#define TAMA_HUB_AGENT_DEFAULT {cstr(agent_default)}', '']

    lines += [f'#define {GAME_MACRO[g]} 1' for g in games]
    lines += [f'#define {APP_MACRO[a]} 1' for a in apps]
    if buddy:
        lines += ['#define TAMA_ENABLE_BUDDY 1']

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

    write(os.path.join(out_dir, "brand.gen.h"), "\n".join(lines))
