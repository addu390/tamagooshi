from dataclasses import dataclass
from typing import Mapping

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
