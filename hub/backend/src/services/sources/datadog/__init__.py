from __future__ import annotations

from ..registry import SourceProvider, register
from .config import DatadogMetric, DatadogSourceConfig
from .source import DatadogSource

register(SourceProvider(
    "datadog", DatadogSourceConfig, DatadogSource.from_config,
    label="Datadog",
    description="Infrastructure and APM metrics through Datadog timeseries queries.",
))

__all__ = ["DatadogMetric", "DatadogSourceConfig", "DatadogSource"]
