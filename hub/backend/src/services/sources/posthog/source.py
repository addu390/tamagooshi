from __future__ import annotations

import httpx

from ..env import require_env
from ..polling import PollingSource
from .config import PosthogMetric, PosthogSourceConfig


def _insight_value(result: list) -> float:
    first = result[0]
    aggregated = first.get("aggregated_value")
    if aggregated is not None:
        return float(aggregated)
    return float(first["data"][-1])


class PosthogSource(PollingSource):
    """Reads PostHog metrics via a HogQL query or a saved insight.

    Each metric supplies exactly one of `query` (HogQL returning a single
    scalar) or `insight` (an insight short_id, e.g. `mmtixzFF`).
    """

    name = "posthog"

    def __init__(self, cfg: PosthogSourceConfig, api_key: str):
        super().__init__(cfg.interval_secs, cfg.metrics)
        self._base = f"{cfg.host.rstrip('/')}/api/projects/{cfg.project_id}"
        self._headers = {"Authorization": f"Bearer {api_key}"}

    @classmethod
    def from_config(cls, cfg: PosthogSourceConfig) -> PosthogSource:
        return cls(cfg, require_env(cfg.api_key_env))

    async def fetch(self, client: httpx.AsyncClient, metric: PosthogMetric) -> float:
        if metric.insight:
            return await self._fetch_insight(client, metric.insight)
        return await self._fetch_hogql(client, metric.query)

    async def _fetch_insight(self, client: httpx.AsyncClient, short_id: str) -> float:
        resp = await client.get(
            f"{self._base}/insights/",
            params={"short_id": short_id, "refresh": "blocking"},
            headers=self._headers,
        )
        resp.raise_for_status()
        return _insight_value(resp.json()["results"][0]["result"])

    async def _fetch_hogql(self, client: httpx.AsyncClient, query: str) -> float:
        resp = await client.post(
            f"{self._base}/query/",
            json={"query": {"kind": "HogQLQuery", "query": query}},
            headers=self._headers,
        )
        resp.raise_for_status()
        return float(resp.json()["results"][0][0])
