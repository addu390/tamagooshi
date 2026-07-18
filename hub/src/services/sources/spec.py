from __future__ import annotations

from typing import Literal

from pydantic import BaseModel


class MetricSpec(BaseModel):
    key: str
    label: str
    fmt: str = "{v}"
    kind: Literal["normal", "star"] = "normal"


class SourceConfigBase(BaseModel):
    type: str
    interval_secs: float = 3.0
