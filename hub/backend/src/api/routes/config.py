from __future__ import annotations

from fastapi import APIRouter, HTTPException, Request

from ..dependencies import brand_id, brands
from ..lifecycle import apply_change

router = APIRouter()


@router.get("/api/config")
async def config(request: Request):
    return brands(request).read_manifest(brand_id(request))


@router.put("/api/config/identity")
async def put_identity(request: Request):
    identity = await request.json()
    service, bid = brands(request), brand_id(request)
    return apply_change(lambda: service.update_identity(bid, identity))


@router.put("/api/config/device")
async def put_device(request: Request):
    device = await request.json()
    if not isinstance(device, dict):
        raise HTTPException(status_code=400, detail="device section must be an object")

    service, bid = brands(request), brand_id(request)
    return apply_change(lambda: service.update_device(bid, device))
