from __future__ import annotations

import os

from fastapi import APIRouter, HTTPException, Request

from ..config.secrets import delete_secret, load_secrets, set_secret
from .deps import hub_config

router = APIRouter()


@router.get("/api/secrets")
async def secrets(request: Request):
    stored = load_secrets()
    referenced: dict[str, dict] = {}

    for source in hub_config(request).sources:
        for name in source.secret_envs():
            referenced[name] = {
                "name": name,
                "set": bool(os.environ.get(name)),
                "stored": name in stored,
            }

    for name in stored:
        referenced.setdefault(name, {"name": name, "set": True, "stored": True})

    return sorted(referenced.values(), key=lambda s: s["name"])


@router.put("/api/secrets")
async def put_secret(request: Request):
    body = await request.json()
    name = str(body.get("name", "")).strip()
    value = str(body.get("value", ""))

    if not name or not name.replace("_", "").isalnum():
        raise HTTPException(status_code=400, detail="invalid secret name")
    if not value:
        raise HTTPException(status_code=400, detail="value required")

    set_secret(name, value)
    return {"name": name, "set": True}


@router.delete("/api/secrets/{name}")
async def remove_secret(name: str):
    delete_secret(name)
    return {"name": name, "set": False}
