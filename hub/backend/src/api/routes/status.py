from __future__ import annotations

from fastapi import APIRouter, Request

from ..dependencies import hub_config

router = APIRouter()


@router.get("/healthz")
async def healthz(request: Request):
    return {"status": "ok", "device_id": hub_config(request).device_id}


@router.get("/api/devices")
async def devices(request: Request):
    return request.app.state.inbound.snapshot()


@router.get("/api/status")
async def status(request: Request):
    state = request.app.state
    config = hub_config(request)
    return {
        "device_id": config.device_id,
        "brand": config.brand.model_dump(),
        "devices": state.inbound.snapshot(),
    }
