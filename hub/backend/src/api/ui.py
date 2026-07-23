from __future__ import annotations

import os
import sys

from fastapi import FastAPI
from fastapi.responses import FileResponse, RedirectResponse
from fastapi.staticfiles import StaticFiles


def _repo() -> str:
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(here, "..", "..", "..", ".."))


def _ui_dir() -> str | None:
    override = os.environ.get("TAMA_UI_DIR")
    if override:
        return override
    bundled = os.path.join(getattr(sys, "_MEIPASS", ""), "ui")
    if bundled != "ui" and os.path.isdir(bundled):
        return bundled
    local = os.path.join(_repo(), "hub", "console")
    return local if os.path.isdir(local) else None


def _tokens_path(ui_dir: str) -> str | None:
    for candidate in (os.path.join(ui_dir, "tokens.css"),
                      os.path.join(_repo(), "website", "common", "css", "tokens.css")):
        if os.path.isfile(candidate):
            return candidate
    return None


def mount_ui(app: FastAPI) -> None:
    ui_dir = _ui_dir()
    if ui_dir is None:
        return

    @app.get("/", include_in_schema=False)
    async def root():
        return RedirectResponse(url="/ui/")

    tokens = _tokens_path(ui_dir)
    if tokens is not None:
        @app.get("/ui/tokens.css", include_in_schema=False)
        async def tokens_css():
            return FileResponse(tokens, media_type="text/css")

    app.mount("/ui", StaticFiles(directory=ui_dir, html=True), name="ui")
