from collections.abc import Mapping
from dataclasses import dataclass

from gen.features.apps import APPS
from gen.features.games import GAMES
from gen.features.mascots import MASCOTS
from gen.ui.themes import THEMES
from gen.ui.typefaces import TYPEFACES


@dataclass(frozen=True)
class Category:
    id: str
    noun: str
    items: Mapping[str, Mapping]
    macro_prefix: str = ""

    def macro(self, item_id):
        return f"{self.macro_prefix}_{item_id.upper()}"

    def label(self, item_id):
        return self.items[item_id].get("label") or item_id.upper()

    def display(self, item_id):
        meta = self.items[item_id]
        return meta.get("desc") or meta.get("label") or item_id

    def selectable(self):
        return [i for i, meta in self.items.items() if not meta.get("soon")]

    def rows(self):
        return [[i, self.display(i)] for i in self.selectable()]


apps = Category("apps", "app", APPS, "TAMA_APP")
games = Category("games", "game", GAMES, "TAMA_GAME")
themes = Category("themes", "theme", THEMES)
typefaces = Category("typefaces", "typeface", TYPEFACES)
mascots = Category("mascots", "mascot", MASCOTS)

HID_KINDS = ("gamepad", "media", "keyboard", "mouse")

HID_MODES = {
    "gamepad": {"label": "Gamepad", "capabilities": ("gamepad",)},
    "desk": {"label": "Desk", "capabilities": ("media", "keyboard", "mouse")},
    "off": {"label": "Off", "capabilities": ()},
}


def hid_bits(capabilities):
    bits = 0
    for kind in capabilities:
        bits |= 1 << HID_KINDS.index(kind)
    return bits


def hid_macros(game_ids, app_ids):
    used = ({games.items[i].get("hid") for i in game_ids}
            | {apps.items[i].get("hid") for i in app_ids})
    kinds = [k for k in HID_KINDS if k in used]
    return [f"TAMA_NEEDS_HID={int(bool(kinds))}"]


def hid_mode_rows():
    return [[mode_id, meta["label"], list(meta["capabilities"])]
            for mode_id, meta in HID_MODES.items()]
