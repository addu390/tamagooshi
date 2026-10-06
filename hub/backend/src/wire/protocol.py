from __future__ import annotations

import json
import os
import time
from pydantic import BaseModel, ValidationError

PROTOCOL_VERSION = 1

_CROCKFORD = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"


def ulid(now_ms: int | None = None) -> str:
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


class Branding(BaseModel):
    name: str
    tagline: str | None = None
    logo_id: str | None = None


class ConfigSet(BaseModel):
    theme: str | None = None
    character_id: str | None = None
    carousel_secs: int | None = None
    hid_mode: str | None = None


class TimeSet(BaseModel):
    epoch: int
    tz_offset: int = 0


def envelope(type_: str, body: BaseModel, src: str = "hub", now_ms: int | None = None) -> str:
    env = Envelope(
        type=type_,
        id=ulid(now_ms),
        ts=int((now_ms / 1000) if now_ms is not None else time.time()),
        src=src,
        body=body.model_dump(exclude_none=True),
    )
    return json.dumps(env.model_dump(), separators=(",", ":"))


def parse_envelope(payload: str) -> Envelope | None:
    try:
        data = json.loads(payload)
    except (json.JSONDecodeError, TypeError):
        return None
    try:
        return Envelope.model_validate(data)
    except ValidationError:
        return None
