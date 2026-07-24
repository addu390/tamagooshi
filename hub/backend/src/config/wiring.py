from __future__ import annotations

import os
import sys

from .catalog import LayeredCatalog
from .settings import user_brands_dir
from .sources import FileTreeSource, FileTreeStore


def builtin_brands_dir() -> str:
    override = os.environ.get("TAMA_BRANDS_DIR")
    if override:
        return override
    bundled = os.path.join(getattr(sys, "_MEIPASS", ""), "brands")
    if bundled != "brands" and os.path.isdir(bundled):
        return bundled
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(here, "..", "..", "..", "..", "brands"))


def default_catalog() -> LayeredCatalog:
    return LayeredCatalog(FileTreeStore(user_brands_dir()), FileTreeSource(builtin_brands_dir()))
