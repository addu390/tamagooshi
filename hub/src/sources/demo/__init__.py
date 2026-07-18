from __future__ import annotations

from ..registry import SourceProvider, register
from .config import DemoMetric, DemoSourceConfig, Incident
from .source import DemoSource

register(SourceProvider("demo", DemoSourceConfig, DemoSource))

__all__ = ["DemoMetric", "DemoSourceConfig", "Incident", "DemoSource"]
