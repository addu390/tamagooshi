from __future__ import annotations

import os

from fastapi import APIRouter, HTTPException, Request

from ..config import store
from ..services.sources import (
    CollectError, build_source, collect_once, parse_sources, provider, providers,
)
from ..services.worker import SourceRuntime
from .deps import brand_id, worker
from .lifecycle import apply_change

router = APIRouter()


def _to_json(runtime: SourceRuntime) -> dict:
    cfg = runtime.config.model_dump()
    status = runtime.source.metric_status() if runtime.source else {}

    metrics = []
    for m in cfg.get("metrics", []):
        st = status.get(m["key"], {})
        metrics.append({
            "key": m["key"],
            "label": m["label"],
            "query": m.get("query") or m.get("insight") or "",
            "value": st.get("value"),
            "error": st.get("error"),
            "last_ts": st.get("ts"),
        })

    return {
        "index": runtime.index,
        "type": cfg["type"],
        "label": provider(cfg["type"]).label or cfg["type"],
        "enabled": cfg.get("enabled", True),
        "running": runtime.running,
        "interval_secs": cfg.get("interval_secs"),
        "error": runtime.error,
        "last_emit": runtime.last_emit,
        "metrics": metrics,
        "secrets": [{"name": name, "set": bool(os.environ.get(name))}
                    for name in runtime.config.secret_envs()],
        "config": cfg,
    }


def _runtime(request: Request, index: int) -> SourceRuntime:
    runtimes = worker(request).snapshot()
    if not 0 <= index < len(runtimes):
        raise HTTPException(status_code=404, detail=f"no source at index {index}")
    return runtimes[index]


def _parse(config: dict):
    try:
        return parse_sources([config])[0]
    except Exception as err:  # noqa: BLE001 - reported as a validation failure
        raise HTTPException(status_code=400, detail=str(err)) from err


@router.get("/api/sources")
async def sources(request: Request):
    return [_to_json(r) for r in worker(request).snapshot()]


@router.get("/api/sources/types")
async def source_types():
    return [{"type": p.type, "label": p.label or p.type, "description": p.description,
             "schema": p.config_model.model_json_schema()}
            for p in providers()]


@router.post("/api/sources/test")
async def test_source(request: Request):
    cfg = _parse(await request.json())
    try:
        source = build_source(cfg)
    except Exception as err:  # noqa: BLE001 - reported as a test failure
        raise HTTPException(status_code=400, detail=str(err)) from err

    try:
        collected = await collect_once(source)
    except CollectError as err:
        raise HTTPException(status_code=400, detail=str(err)) from err

    if not collected:
        raise HTTPException(status_code=400, detail="no metrics returned, check queries")
    return {"metrics": [{"key": u.key, "label": u.label, "value": u.value} for u in collected]}


@router.post("/api/sources")
async def add_source(request: Request):
    config = await request.json()
    _parse(config)

    bid = brand_id(request)
    return apply_change(lambda: store.add_source(bid, config))


@router.put("/api/sources/{index}")
async def edit_source(request: Request, index: int):
    _runtime(request, index)
    config = await request.json()
    _parse(config)

    bid = brand_id(request)
    return apply_change(lambda: store.update_source(bid, index, config))


@router.delete("/api/sources/{index}")
async def remove_source(request: Request, index: int):
    _runtime(request, index)

    bid = brand_id(request)
    return apply_change(lambda: store.remove_source(bid, index))


@router.post("/api/sources/{index}/toggle")
async def toggle_source(request: Request, index: int):
    _runtime(request, index)
    body = await request.json()
    enabled = bool(body.get("enabled"))

    runtime = worker(request).set_enabled(index, enabled)
    store.set_source_enabled(brand_id(request), index, enabled)
    return _to_json(runtime)


@router.post("/api/sources/{index}/refresh")
async def refresh_source(request: Request, index: int):
    _runtime(request, index)

    runtime = await worker(request).refresh(index)
    return _to_json(runtime)
