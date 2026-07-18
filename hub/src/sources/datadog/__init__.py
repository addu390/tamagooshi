from __future__ import annotations

from ..registry import SourceProvider, register
from .config import DatadogMetric, DatadogSourceConfig
from .source import DatadogSource

register(SourceProvider("datadog", DatadogSourceConfig, DatadogSource.from_config))

__all__ = ["DatadogMetric", "DatadogSourceConfig", "DatadogSource"]
