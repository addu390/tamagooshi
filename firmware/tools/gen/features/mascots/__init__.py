from gen.features.mascots.claude import CLAUDE
from gen.features.mascots.pals import PALS
from gen.features.mascots.poke import POKE
from gen.features.mascots.trucks import TRUCKS

_PACKS = [("pals", PALS), ("trucks", TRUCKS), ("poke", POKE), ("claude", CLAUDE)]

MASCOTS = {m["id"]: m for _, pack in _PACKS for m in pack}
MASCOT_CATEGORIES = {name: [m["id"] for m in pack] for name, pack in _PACKS}

__all__ = ["MASCOTS", "MASCOT_CATEGORIES"]
