from __future__ import annotations

import asyncio
import logging
import time

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

    def __init__(self, interval_secs: float, metrics: list[MetricSpec], timeout_secs: float = 10.0):
        self._interval = interval_secs
        self._metrics = metrics
        self._timeout = timeout_secs
        self._last: dict[str, float] = {}
        self._status: dict[str, dict] = {}

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

    async def poll(self, client: httpx.AsyncClient) -> list[MetricUpdate]:
        updates: list[MetricUpdate] = []

        for metric in self._metrics:
            try:
                value = await self.fetch(client, metric)
            except _FETCH_ERRORS as exc:
                log.warning("%s: fetch failed for %s: %s", self.name, metric.key, exc)
                self._status[metric.key] = {"value": None, "error": str(exc), "ts": time.time()}
                continue

            update = self._to_update(metric, value)
            self._status[metric.key] = {"value": update.value, "error": None, "ts": time.time()}
            updates.append(update)

        return updates

    def metric_status(self) -> dict[str, dict]:
        return dict(self._status)

    async def run(self, emit: Emit) -> None:
        async with httpx.AsyncClient(timeout=self._timeout) as client:
            while True:
                for update in await self.poll(client):
                    await emit(update)
                await asyncio.sleep(self._interval)

    async def poll_once(self, emit: Emit) -> None:
        async with httpx.AsyncClient(timeout=self._timeout) as client:
            for update in await self.poll(client):
                await emit(update)
