from __future__ import annotations

import time

import httpx

from ..env import require_env
from ..polling import PollingSource
from .config import DatadogMetric, DatadogSourceConfig


class DatadogSource(PollingSource):
    """Pulls the latest point of a Datadog timeseries query per metric.

    Each metric's `query` is a Datadog metrics query string, e.g.
    `avg:system.cpu.user{*}` or `sum:app.signups{env:prod}.as_count()`.
    """

    name = "datadog"

    def __init__(self, cfg: DatadogSourceConfig, api_key: str, app_key: str):
        super().__init__(cfg.interval_secs, cfg.metrics)
        self._site = cfg.site
        self._window = cfg.window_secs
        self._api_key = api_key
        self._app_key = app_key

    @classmethod
    def from_config(cls, cfg: DatadogSourceConfig) -> DatadogSource:
        return cls(cfg, require_env(cfg.api_key_env), require_env(cfg.app_key_env))

    async def fetch(self, client: httpx.AsyncClient, metric: DatadogMetric) -> float:
        now = int(time.time())
        resp = await client.get(
            f"https://api.{self._site}/api/v1/query",
            params={"from": now - self._window, "to": now, "query": metric.query},
            headers={"DD-API-KEY": self._api_key, "DD-APPLICATION-KEY": self._app_key},
        )
        resp.raise_for_status()
        pointlist = resp.json()["series"][0]["pointlist"]
        return float(pointlist[-1][1])
