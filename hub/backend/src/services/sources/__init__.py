from __future__ import annotations

from . import (  # noqa: F401  (import triggers provider self-registration)
    datadog,
    demo,
    posthog,
)
from .base import Source
from .oneshot import CollectError, collect_once
from .polling import PollingSource
from .registry import (
    SourceProvider,
    build_source,
    parse_sources,
    provider,
    providers,
    register,
)
from .spec import MetricSpec, SourceConfigBase

__all__ = [
    "CollectError",
    "MetricSpec",
    "PollingSource",
    "Source",
    "SourceConfigBase",
    "SourceProvider",
    "build_source",
    "collect_once",
    "parse_sources",
    "provider",
    "providers",
    "register",
]
