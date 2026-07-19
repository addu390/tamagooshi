from __future__ import annotations

import logging

import yaml
from fastapi import APIRouter, HTTPException, Request
from fastapi.responses import Response

from ..config import resolve_brand, store
from ..config.settings import set_active_brand
from .deps import brand_id
from .lifecycle import schedule_restart

log = logging.getLogger("tamagooshi.api.brands")

router = APIRouter()


@router.get("/api/brands")
async def brands(request: Request):
    active = brand_id(request)
    return [{**entry, "active": entry["id"] == active} for entry in store.list_brands()]


@router.post("/api/brands/import")
async def import_brand(request: Request):
    try:
        data = yaml.safe_load(await request.body()) or {}
        bid = store.import_manifest(data)
    except Exception as err:  # noqa: BLE001 - reject any unparsable manifest
        raise HTTPException(status_code=400, detail=f"invalid brand config: {err}") from err

    return {"id": bid}


@router.post("/api/brands")
async def create_brand(request: Request):
    body = await request.json()

    try:
        bid = store.create_brand(
            str(body.get("id") or "").strip(),
            str(body.get("name") or "").strip(),
            str(body.get("tagline") or "").strip(),
        )
    except ValueError as err:
        raise HTTPException(status_code=400, detail=str(err)) from err
    except FileNotFoundError as err:
        raise HTTPException(status_code=500, detail="template brand missing") from err

    log.info("brand %s created from template", bid)
    return {"id": bid}


@router.get("/api/brands/{bid}/manifest")
async def brand_manifest(bid: str):
    try:
        return store.read_manifest(bid)
    except FileNotFoundError as err:
        raise HTTPException(status_code=404, detail=str(err)) from err


@router.get("/api/brands/{bid}/export")
async def export_brand(bid: str):
    try:
        path = resolve_brand(bid)
    except FileNotFoundError as err:
        raise HTTPException(status_code=404, detail=str(err)) from err

    with open(path, "r", encoding="utf-8") as fh:
        content = fh.read()

    return Response(content, media_type="application/x-yaml",
                    headers={"Content-Disposition": f'attachment; filename="{bid}.yaml"'})


@router.delete("/api/brands/{bid}")
async def delete_brand(request: Request, bid: str):
    if store.user_copy(bid) is None:
        raise HTTPException(status_code=404, detail="built-in brands cannot be deleted")

    active = bid == brand_id(request)
    if active and not store.builtin_exists(bid):
        raise HTTPException(status_code=400,
                            detail="activate another brand before deleting this one")

    store.delete_user_brand(bid)

    if active:
        log.info("user copy of active brand %s deleted; restarting hub", bid)
        schedule_restart()
        return {"id": bid, "restarting": True}

    log.info("brand %s deleted", bid)
    return {"id": bid, "restarting": False}


@router.post("/api/brands/activate")
async def activate(request: Request):
    body = await request.json()
    bid = body.get("brand")
    try:
        resolve_brand(bid)
    except FileNotFoundError as err:
        raise HTTPException(status_code=404, detail=str(err)) from err

    set_active_brand(bid)

    log.info("brand set to %s; restarting hub", bid)
    schedule_restart()
    return {"brand": bid, "restarting": True}
