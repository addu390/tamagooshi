from __future__ import annotations

from typing import Literal

from pydantic import BaseModel, Field

from ..spec import MetricSpec, SourceConfigBase


class Incident(BaseModel):
    every_secs: float
    for_secs: float
    value: float


class Keyframe(BaseModel):
    at: float
    value: float


class DemoMetric(MetricSpec):
    value_start: float
    drift: float = 0.0
    incident: Incident | None = None
    timeline: list[Keyframe] | None = None


class DemoSourceConfig(SourceConfigBase):
    type: Literal["demo"] = "demo"
    metrics: list[DemoMetric] = Field(default_factory=list)
