from __future__ import annotations

from dataclasses import dataclass
from typing import Literal, Optional

Mood = Literal["happy", "neutral", "sick", "panic", "celebrate", "sleepy"]
Severity = Literal["info", "warning", "critical"]
Op = Literal["lt", "lte", "gt", "gte", "eq", "ne"]
MetricKind = Literal["normal", "star"]


@dataclass(frozen=True)
class MetricUpdate:
    key: str
    label: str
    value: str
    kind: MetricKind = "normal"
    trend: Optional[str] = None
    raw: Optional[float] = None
