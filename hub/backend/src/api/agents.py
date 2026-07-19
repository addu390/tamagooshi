from __future__ import annotations

import importlib.util
import shutil

from fastapi import APIRouter, HTTPException, Request

from ..config import store
from ..features.buddy.agents.catalog import CATALOG
from .deps import brand_id, hub_config
from .lifecycle import apply_change

router = APIRouter()


@router.get("/api/agents")
async def agents(request: Request):
    cfg = hub_config(request).agent
    out = []
    for aid, entry in CATALOG.items():
        out.append({
            "id": aid,
            "label": entry["label"],
            "enabled": aid in cfg.enabled,
            "default": aid == cfg.default,
            "cli": shutil.which(entry["cli"]),
            "sdk": importlib.util.find_spec(entry["sdk"]) is not None,
        })
    return out


@router.put("/api/agents")
async def put_agents(request: Request):
    body = await request.json()
    default = str(body.get("default", ""))
    enabled = list(body.get("enabled") or [])

    unknown = [aid for aid in enabled + [default] if aid not in CATALOG]
    if unknown:
        raise HTTPException(status_code=400, detail=f"unknown agent(s): {', '.join(unknown)}")
    if enabled and default not in enabled:
        raise HTTPException(status_code=400, detail="default agent must be enabled")

    bid = brand_id(request)
    return apply_change(lambda: store.update_agent(bid, default, enabled))
