from __future__ import annotations

import asyncio
import logging
from dataclasses import asdict

from fastapi import APIRouter, HTTPException, Request

from ...config.settings import load_connection, save_connection
from ...network.transport.factory import resolve_spec, spec_locked, transport_spec
from ..dependencies import transport
from ..lifecycle import apply_change

log = logging.getLogger("tamagooshi.api.connection")

router = APIRouter()

SCAN_TIMEOUT = 6.0


async def _apply_link_change(request: Request, mutation) -> dict:
    try:
        await asyncio.to_thread(transport(request).close)
    except Exception:
        log.exception("failed to close current link before restart")
    return apply_change(mutation)


@router.get("/api/connection")
async def connection(request: Request):
    link, protocol, _ = resolve_spec(transport_spec())
    return {
        "link": link,
        "protocol": protocol,
        "locked": spec_locked(),
        "saved": load_connection().get("device"),
        **asdict(transport(request).status()),
    }


@router.post("/api/connection/scan")
async def scan():
    from ...network.transport.ble import discover

    try:
        devices = await discover(SCAN_TIMEOUT)
    except Exception as err:
        raise HTTPException(status_code=503, detail=f"bluetooth scan failed: {err}") from err

    return {"devices": devices}


@router.put("/api/connection")
async def put_connection(request: Request):
    body = await request.json()
    if not isinstance(body, dict):
        raise HTTPException(status_code=400, detail="connection must be an object")

    spec = str(body.get("transport") or "ble")
    device = body.get("device")
    if device is not None and not isinstance(device, dict):
        raise HTTPException(status_code=400, detail="device must be an object")

    def save():
        link, protocol, _ = resolve_spec(spec)
        saved = {"transport": f"{link}:{protocol}"}
        if device:
            saved["device"] = {"name": device.get("name"), "address": device.get("address")}
        save_connection(saved)

    return await _apply_link_change(request, save)


@router.delete("/api/connection/device")
async def forget_device(request: Request):
    def forget():
        saved = load_connection()
        saved.pop("device", None)
        save_connection(saved)

    return await _apply_link_change(request, forget)
