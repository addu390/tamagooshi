import os
import re

FLAGS = ("imu", "mic", "joystick", "ir")

APP_TEMPLATE = """#include "brand.gen.h"
#if TAMA_APP_{macro}

#include "apps.h"
#include "theme.h"
#include "widgets.h"

namespace tama::apps {{

namespace {{

class {cls}Screen : public AppScreen {{
 public:
  const char* id() const override {{ return "app.{iid}"; }}

  void render(Gfx& g, ShellContext& ctx) override {{
    const auto L = widgets::frame(g, ctx.state, "{label}");
    g.str("{iid}", L.cx, L.cy, theme::kDim, typeface::body(), textdatum_t::middle_center);
    widgets::hints(g, nullptr, "BACK");
  }}

  Transition handleInput(Intent intent, ShellContext&) override {{
    if (intent == Intent::Next) return Transition::back();
    return Transition::none();
  }}
}};

}}  // namespace

TAMA_SCREEN_FACTORY({iid}, {cls}Screen)

}}  // namespace tama::apps

#endif  // TAMA_APP_{macro}
"""

GAME_TEMPLATE = """#include "brand.gen.h"
#if TAMA_GAME_{macro}

#include "arcade.h"
#include "games.h"

namespace tama::games {{

namespace {{

class {cls}Screen : public ArcadeGameScreen {{
 public:
  {cls}Screen() : ArcadeGameScreen(OrientationPref::Portrait) {{}}
  const char* id() const override {{ return "game.{iid}"; }}

 protected:
  const char* title() const override {{ return "{label}"; }}
  const char* readyHint() const override {{ return "A TO START"; }}
  const char* runHint() const override {{ return "GO"; }}

  void renderWorld(Gfx& g, ShellContext& ctx) override {{
    player(g, ctx, w_ / 2, h_ / 2, 28, Expr::Neutral);
  }}

  void onReset() override {{}}

  void step(ShellContext&) override {{}}
}};

}}  // namespace

TAMA_SCREEN_FACTORY({iid}, {cls}Screen)

}}  // namespace tama::games

#endif  // TAMA_GAME_{macro}
"""


def _firmware_dir():
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def _check_id(iid):
    if not re.fullmatch(r"[a-z][a-z0-9_]*", iid):
        raise SystemExit(f"invalid id '{iid}': use lowercase letters, digits, underscores")


def _insert_entry(path, iid, entry):
    with open(path, "r", encoding="utf-8") as fh:
        text = fh.read()
    if re.search(rf'^\s*"{iid}":', text, re.M):
        raise SystemExit(f"'{iid}' already registered in {os.path.basename(path)}")
    at = text.rstrip().rfind("\n}")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(text[:at] + "\n" + entry + text[at:])


def _write_new(path, content):
    if os.path.exists(path):
        raise SystemExit(f"{path} already exists")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(content)


def _feature(kind, iid, desc, flags):
    firmware = _firmware_dir()
    template = APP_TEMPLATE if kind == "app" else GAME_TEMPLATE

    parts = [f'"label": "{iid.upper()}"', f'"desc": "{desc}"']
    parts += [f'"{f}": True' for f in FLAGS if f in flags]
    registry = os.path.join(firmware, "tools", "gen", "features", f"{kind}s.py")
    _insert_entry(registry, iid, f'    "{iid}": {{{", ".join(parts)}}},')

    cls = "".join(p.capitalize() for p in iid.split("_"))
    stub = os.path.join(firmware, "lib", "features", f"{kind}s", f"{iid}.cpp")
    _write_new(stub, template.format(iid=iid, cls=cls, macro=iid.upper(), label=iid.upper()))

    return [registry, stub]


def _theme(iid, surface, ink, accent):
    firmware = _firmware_dir()
    registry = os.path.join(firmware, "tools", "gen", "ui", "themes.py")
    accent_part = f', "accent": "{accent}"' if accent else ""
    entry = f'    "{iid}": _derived({{"surface": "{surface}", "ink": "{ink}"{accent_part}}}),'
    _insert_entry(registry, iid, entry)
    return [registry]


def new(args):
    kinds = ("app", "game", "theme")
    if len(args) < 2 or args[0] not in kinds:
        raise SystemExit(
            "usage: python3 -m gen new app|game <id> <description> [imu] [mic] [joystick] [ir]\n"
            "       python3 -m gen new theme <id> <surface-hex> <ink-hex> [accent-hex]")
    kind, iid = args[0], args[1]
    _check_id(iid)

    if kind == "theme":
        if len(args) < 4:
            raise SystemExit("usage: python3 -m gen new theme <id> <surface-hex> <ink-hex> "
                             "[accent-hex]")
        touched = _theme(iid, args[2], args[3], args[4] if len(args) > 4 else None)
    else:
        if len(args) < 3:
            raise SystemExit(f"usage: python3 -m gen new {kind} <id> <description> "
                             "[imu] [mic] [joystick] [ir]")
        touched = _feature(kind, iid, args[2], set(args[3:]))

    for path in touched:
        print(f"wrote {path}")
    print(f"next: enable '{iid}' in a brand yaml, then run: python3 -m gen catalog")
