from __future__ import annotations

from typing import Literal

from pydantic import Field, model_validator

from ..spec import MetricSpec, SourceConfigBase


class PosthogMetric(MetricSpec):
    query: str | None = None
    insight: str | None = None

    @model_validator(mode="after")
    def _one_source(self) -> PosthogMetric:
        if bool(self.query) == bool(self.insight):
            raise ValueError("posthog metric needs exactly one of 'query' or 'insight'")
        return self


class PosthogSourceConfig(SourceConfigBase):
    type: Literal["posthog"] = "posthog"
    host: str = "https://us.posthog.com"
    project_id: str
    api_key_env: str = "POSTHOG_API_KEY"
    metrics: list[PosthogMetric] = Field(default_factory=list)
