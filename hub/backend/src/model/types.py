from __future__ import annotations

from typing import Literal

Mood = Literal["happy", "neutral", "sick", "panic", "celebrate", "sleepy"]
Severity = Literal["info", "warning", "critical"]
Op = Literal["lt", "lte", "gt", "gte", "eq", "ne"]
