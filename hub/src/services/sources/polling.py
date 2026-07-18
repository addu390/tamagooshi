from __future__ import annotations

import asyncio
import logging
from typing import Dict, List

import httpx

from ...model import MetricUpdate
from .base import Emit, Source
from .format import humanize, trend
from .spec import MetricSpec

log = logging.getLogger("tamagooshi.sources")

_FETCH_ERRORS = (httpx.HTTPError, ValueError, KeyError, IndexError, TypeError)


class PollingSource(Source):
    """Base for sources that periodically pull metric values over HTTP.

    Subclasses implement `fetch()` to return the latest numeric value for one
    metric. This base owns the poll loop, per-key trend tracking, value
    formatting, and isolates a single failing metric from the rest.
    """

    name = "polling"

    def __init__(self, interval_secs: float, metrics: List[MetricSpec], timeout_secs: float = 10.0):
        self._interval = interval_secs
        self._metrics = metrics
        self._timeout = timeout_secs
        self._last: Dict[str, float] = {}

    async def fetch(self, client: httpx.AsyncClient, metric: MetricSpec) -> float:
        raise NotImplementedError

    def _to_update(self, metric: MetricSpec, value: float) -> MetricUpdate:
        delta = value - self._last.get(metric.key, value)
        self._last[metric.key] = value
        return MetricUpdate(
            key=metric.key,
            label=metric.label,
            value=metric.fmt.format(v=humanize(value)),
            kind=metric.kind,
            trend=trend(delta),
            raw=value,
        )

    async def poll(self, client: httpx.AsyncClient) -> List[MetricUpdate]:
        updates: List[MetricUpdate] = []
        for metric in self._metrics:
            try:
                value = await self.fetch(client, metric)
            except _FETCH_ERRORS as exc:
                log.warning("%s: fetch failed for %s: %s", self.name, metric.key, exc)
                continue
            updates.append(self._to_update(metric, value))
        return updates

    async def run(self, emit: Emit) -> None:
        async with httpx.AsyncClient(timeout=self._timeout) as client:
            while True:
                for update in await self.poll(client):
                    await emit(update)
                await asyncio.sleep(self._interval)
