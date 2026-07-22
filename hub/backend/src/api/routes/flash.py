from __future__ import annotations

import base64
import binascii

from fastapi import APIRouter, HTTPException, Request

from ...services import flash
from ..dependencies import flasher

# Same as RELEASE_BASE in firmware/tools/gen/platform/boards.py
RELEASE_PREFIX = "https://github.com/addu390/tamagooshi/releases/latest/download/"

router = APIRouter()


@router.get("/api/flash")
async def flash_status(request: Request):
    return {
        "available": flash.available(),
        "ports": flash.list_ports(),
        "busy": flasher(request).busy,
    }


@router.post("/api/flash")
async def start_flash(request: Request):
    body = await request.json()
    port = body.get("port")
    image_url = body.get("image_url", "")
    if not port:
        raise HTTPException(status_code=400, detail="port is required")
    if not image_url.startswith(RELEASE_PREFIX):
        raise HTTPException(status_code=400, detail="image_url must point at a release build")

    config_offset = body.get("config_offset")
    config_blob = None
    if body.get("config_b64") is not None:
        if config_offset is None:
            raise HTTPException(status_code=400, detail="config_offset is required with config_b64")
        try:
            config_blob = base64.b64decode(body["config_b64"], validate=True)
        except (binascii.Error, ValueError) as err:
            raise HTTPException(status_code=400, detail="config_b64 is not valid base64") from err

    try:
        job = flasher(request).start(port, image_url,
                                     config_offset=config_offset, config_blob=config_blob)
    except RuntimeError as err:
        raise HTTPException(status_code=409, detail=str(err)) from err

    return {"job": job}
