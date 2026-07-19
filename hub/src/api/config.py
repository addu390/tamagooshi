from __future__ import annotations

from fastapi import APIRouter, HTTPException, Request

from ..config import store
from .deps import brand_id
from .lifecycle import apply_change

router = APIRouter()


@router.get("/api/config")
async def config(request: Request):
    return store.read_manifest(brand_id(request))


@router.put("/api/config/identity")
async def put_identity(request: Request):
    identity = await request.json()
    bid = brand_id(request)
    return apply_change(lambda: store.update_identity(bid, identity))


@router.put("/api/config/device")
async def put_device(request: Request):
    device = await request.json()
    if not isinstance(device, dict):
        raise HTTPException(status_code=400, detail="device section must be an object")

    bid = brand_id(request)
    return apply_change(lambda: store.update_device(bid, device))
