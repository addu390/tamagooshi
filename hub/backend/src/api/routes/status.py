from __future__ import annotations

from dataclasses import asdict

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
    pipeline = state.pipeline
    config = hub_config(request)
    acked = pipeline.alerts.snapshot().get("acknowledged", [])
    return {
        "device_id": config.device_id,
        "brand": config.brand.model_dump(),
        "mood": pipeline.current_mood,
        "metrics": [asdict(m) for m in pipeline.metric_details()],
        "alerts": [{"id": r.id, "title": r.title, "severity": r.severity,
                    "acked": r.id in acked} for r in pipeline.active_alerts()],
        "devices": state.inbound.snapshot(),
        "bridge": state.bridge is not None,
    }
