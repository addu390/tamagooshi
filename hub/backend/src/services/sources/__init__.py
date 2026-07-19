from __future__ import annotations

from .base import Source
from .oneshot import CollectError, collect_once
from .polling import PollingSource
from .registry import SourceProvider, build_source, parse_sources, provider, providers, register
from .spec import MetricSpec, SourceConfigBase

from . import datadog, demo, posthog  # noqa: F401  (import triggers provider self-registration)

__all__ = [
    "Source", "PollingSource", "MetricSpec", "SourceConfigBase",
    "SourceProvider", "register", "parse_sources", "provider", "providers", "build_source",
    "CollectError", "collect_once",
]
