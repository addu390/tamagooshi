from __future__ import annotations

from typing import get_args

from fastapi import APIRouter, HTTPException, Request

from ...model.types import Mood, Op, Severity
from ..dependencies import brand_id, brands
from ..lifecycle import apply_change

router = APIRouter()

OPTIONS = {
    "moods": list(get_args(Mood)),
    "severities": list(get_args(Severity)),
    "ops": list(get_args(Op)),
}


@router.get("/api/rules")
async def rules(request: Request):
    device = brands(request).read_manifest(brand_id(request)).get("device") or {}
    return {
        "default_mood": (device.get("mascot") or {}).get("mood", "happy"),
        "moods": device.get("moods") or [],
        "alerts": device.get("alerts") or [],
        "options": OPTIONS,
    }


async def _rule_list(request: Request) -> list:
    body = await request.json()
    if not isinstance(body, list):
        raise HTTPException(status_code=400, detail="expected a list of rules")
    return body


@router.put("/api/rules/moods")
async def put_moods(request: Request):
    moods = await _rule_list(request)
    service, bid = brands(request), brand_id(request)
    return apply_change(lambda: service.update_rules(bid, moods=moods))


@router.put("/api/rules/alerts")
async def put_alerts(request: Request):
    alerts = await _rule_list(request)
    service, bid = brands(request), brand_id(request)
    return apply_change(lambda: service.update_rules(bid, alerts=alerts))
