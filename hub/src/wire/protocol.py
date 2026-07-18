from __future__ import annotations

import json
import os
import time
from typing import Literal, Optional

from pydantic import BaseModel, ValidationError

from ..model import Mood, Severity

PROTOCOL_VERSION = 1

_CROCKFORD = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"


def ulid(now_ms: Optional[int] = None) -> str:
    ms = int(time.time() * 1000) if now_ms is None else now_ms
    rand = int.from_bytes(os.urandom(10), "big")
    value = (ms << 80) | rand
    out = bytearray(26)
    for i in range(25, -1, -1):
        out[i] = ord(_CROCKFORD[value & 0x1F])
        value >>= 5
    return out.decode("ascii")


class Envelope(BaseModel):
    v: int = PROTOCOL_VERSION
    type: str
    id: str
    ts: int
    src: str = "hub"
    body: dict


class MetricUpsert(BaseModel):
    key: str
    label: str
    value: str
    trend: Optional[str] = None
    kind: Literal["normal", "star"] = "normal"
    ts: Optional[int] = None


class Branding(BaseModel):
    name: str
    tagline: Optional[str] = None
    logo_id: Optional[str] = None


class MoodSet(BaseModel):
    state: Mood
    reason: Optional[str] = None


class PageRaise(BaseModel):
    id: str
    title: str
    severity: Severity = "warning"
    body: Optional[str] = None
    source: Optional[str] = None
    requires_ack: bool = True


class PageRef(BaseModel):
    id: str


class ConfigSet(BaseModel):
    theme: Optional[str] = None
    character_id: Optional[str] = None
    carousel_secs: Optional[int] = None


class TimeSet(BaseModel):
    epoch: int
    tz_offset: int = 0


class PageAck(BaseModel):
    page_id: str
    by: str = "device"


def envelope(type_: str, body: BaseModel, src: str = "hub", now_ms: Optional[int] = None) -> str:
    env = Envelope(
        type=type_,
        id=ulid(now_ms),
        ts=int((now_ms / 1000) if now_ms is not None else time.time()),
        src=src,
        body=body.model_dump(exclude_none=True),
    )
    return json.dumps(env.model_dump(), separators=(",", ":"))


def parse_envelope(payload: str) -> Optional[Envelope]:
    try:
        data = json.loads(payload)
    except (json.JSONDecodeError, TypeError):
        return None
    try:
        return Envelope.model_validate(data)
    except ValidationError:
        return None
