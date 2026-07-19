from __future__ import annotations

from pydantic import BaseModel

from .types import Mood, Op, Severity


class Condition(BaseModel):
    metric: str
    op: Op
    value: float


class MoodRule(BaseModel):
    when: Condition
    mood: Mood
    priority: int = 0


class AlertRule(BaseModel):
    id: str
    when: Condition
    severity: Severity = "warning"
    title: str
    body: str = ""
    source: str = "hub"
    requires_ack: bool = True
