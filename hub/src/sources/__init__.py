from __future__ import annotations

from .base import Source
from .polling import PollingSource
from .registry import SourceProvider, build_source, parse_sources, register
from .spec import MetricSpec, SourceConfigBase

from . import datadog, demo, posthog  # noqa: F401  (import triggers provider self-registration)

__all__ = [
    "Source", "PollingSource", "MetricSpec", "SourceConfigBase",
    "SourceProvider", "register", "parse_sources", "build_source",
]
