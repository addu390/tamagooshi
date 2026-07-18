from __future__ import annotations

from typing import List, Literal

from pydantic import Field

from ..spec import MetricSpec, SourceConfigBase


class DatadogMetric(MetricSpec):
    query: str


class DatadogSourceConfig(SourceConfigBase):
    type: Literal["datadog"] = "datadog"
    site: str = "datadoghq.com"
    window_secs: int = 900
    api_key_env: str = "DATADOG_API_KEY"
    app_key_env: str = "DATADOG_APP_KEY"
    metrics: List[DatadogMetric] = Field(default_factory=list)
