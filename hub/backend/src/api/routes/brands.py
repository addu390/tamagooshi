from __future__ import annotations

import logging

from fastapi import APIRouter, HTTPException, Request
from fastapi.responses import Response

from ...config import BrandNotFound
from ...config.codec import YAML
from ...config.settings import set_active_brand
from ..dependencies import brand_id, brands
from ..lifecycle import schedule_restart

log = logging.getLogger("tamagooshi.api.brands")

router = APIRouter()


@router.get("/api/brands")
async def list_brands(request: Request):
    active = brand_id(request)
    return [{**entry, "active": entry["id"] == active}
            for entry in brands(request).list_brands()]


@router.post("/api/brands/import")
async def import_brand(request: Request):
    try:
        data = YAML.load(await request.body())
        bid = brands(request).import_manifest(data)
    except Exception as err:  # noqa: BLE001 - reject any unparsable manifest
        raise HTTPException(status_code=400, detail=f"invalid brand config: {err}") from err

    return {"id": bid}


@router.post("/api/brands")
async def create_brand(request: Request):
    body = await request.json()

    try:
        bid = brands(request).create_brand(
            str(body.get("id") or "").strip(),
            str(body.get("name") or "").strip(),
            str(body.get("tagline") or "").strip(),
        )
    except ValueError as err:
        raise HTTPException(status_code=400, detail=str(err)) from err
    except BrandNotFound as err:
        raise HTTPException(status_code=500, detail="template brand missing") from err

    log.info("brand %s created from template", bid)
    return {"id": bid}


@router.get("/api/brands/{bid}/manifest")
async def brand_manifest(request: Request, bid: str):
    try:
        return brands(request).read_manifest(bid)
    except BrandNotFound as err:
        raise HTTPException(status_code=404, detail=str(err)) from err


@router.get("/api/brands/{bid}/export")
async def export_brand(request: Request, bid: str):
    try:
        data = brands(request).read_manifest(bid)
    except BrandNotFound as err:
        raise HTTPException(status_code=404, detail=str(err)) from err

    return Response(YAML.dump(data), media_type=YAML.media_type,
                    headers={"Content-Disposition": f'attachment; filename="{bid}.yaml"'})


@router.delete("/api/brands/{bid}")
async def delete_brand(request: Request, bid: str):
    service = brands(request)
    if not service.has_user_copy(bid):
        raise HTTPException(status_code=404, detail="built-in brands cannot be deleted")

    active = bid == brand_id(request)
    if active and not service.builtin_exists(bid):
        raise HTTPException(status_code=400,
                            detail="activate another brand before deleting this one")

    service.delete_user_brand(bid)

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
    if not brands(request).exists(bid):
        raise HTTPException(status_code=404, detail=f"brand '{bid}' not found")

    set_active_brand(bid)

    log.info("brand set to %s; restarting hub", bid)
    schedule_restart()
    return {"brand": bid, "restarting": True}
