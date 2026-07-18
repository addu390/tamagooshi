from mascots.pals import PALS
from mascots.poke import POKE
from mascots.trucks import TRUCKS

_PACKS = [("pals", PALS), ("trucks", TRUCKS), ("poke", POKE)]

MASCOTS = {m["id"]: m for _, pack in _PACKS for m in pack}
MASCOT_CATEGORIES = {name: [m["id"] for m in pack] for name, pack in _PACKS}

__all__ = ["MASCOTS", "MASCOT_CATEGORIES"]
