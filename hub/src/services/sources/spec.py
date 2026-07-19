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
    enabled: bool = True
    interval_secs: float = 3.0

    def secret_envs(self) -> list[str]:
        return [value for key, value in self.model_dump().items() if key.endswith("_env")]
