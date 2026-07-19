from __future__ import annotations

import json

from fastapi import APIRouter, Request
from fastapi.responses import StreamingResponse

router = APIRouter()


async def _stream(request: Request):
    bus = request.app.state.bus
    async for event in bus.listen(replay=True):
        payload = json.dumps({"ts": event.ts, **event.data}, separators=(",", ":"))
        yield f"event: {event.type}\ndata: {payload}\n\n"


@router.get("/api/events")
async def events(request: Request):
    return StreamingResponse(_stream(request), media_type="text/event-stream",
                             headers={"Cache-Control": "no-cache"})
