from __future__ import annotations

from ..registry import SourceProvider, register
from .config import PosthogMetric, PosthogSourceConfig
from .source import PosthogSource

register(SourceProvider(
    "posthog", PosthogSourceConfig, PosthogSource.from_config,
    label="PostHog",
    description="Product analytics from HogQL queries or saved insights.",
))

__all__ = ["PosthogMetric", "PosthogSourceConfig", "PosthogSource"]
