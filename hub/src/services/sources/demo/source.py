from __future__ import annotations

import asyncio
import random
import time
from typing import Callable, List

from ....model import MetricUpdate
from ..base import Emit, Source
from ..format import humanize, trend
from .config import DemoMetric, DemoSourceConfig


class _MetricState:
    def __init__(self, spec: DemoMetric):
        self.spec = spec
        self.baseline = spec.value_start
        self.value = spec.value_start

    def _incident_active(self, elapsed: float) -> bool:
        inc = self.spec.incident
        if inc is None:
            return False
        return (elapsed % inc.every_secs) < inc.for_secs

    def _timeline_value(self, elapsed: float) -> float:
        keys = self.spec.timeline
        period = keys[-1].at
        if period <= 0:
            return keys[0].value
        t = elapsed % period
        prev = keys[0]
        for kf in keys[1:]:
            if t <= kf.at:
                span = kf.at - prev.at
                frac = 0.0 if span <= 0 else (t - prev.at) / span
                return prev.value + (kf.value - prev.value) * frac
            prev = kf
        return keys[-1].value

    def step(self, rng: random.Random, elapsed: float) -> MetricUpdate:
        if self.spec.timeline:
            new_value = self._timeline_value(elapsed)
        else:
            self.baseline = max(0.0, self.baseline + rng.uniform(-self.spec.drift, self.spec.drift))
            new_value = self.spec.incident.value if self._incident_active(elapsed) else self.baseline
        delta = new_value - self.value
        self.value = new_value
        return MetricUpdate(
            key=self.spec.key,
            label=self.spec.label,
            value=self.spec.fmt.format(v=humanize(self.value)),
            kind=self.spec.kind,
            trend=trend(delta),
            raw=self.value,
        )


class DemoSource(Source):
    def __init__(
        self,
        cfg: DemoSourceConfig,
        rng: random.Random | None = None,
        clock: Callable[[], float] | None = None,
    ):
        self._interval = cfg.interval_secs
        self._states: List[_MetricState] = [_MetricState(m) for m in cfg.metrics]
        self._rng = rng or random.Random()
        self._clock = clock or time.monotonic
        self._start = self._clock()

    def tick(self, elapsed: float | None = None) -> List[MetricUpdate]:
        if elapsed is None:
            elapsed = self._clock() - self._start
        return [state.step(self._rng, elapsed) for state in self._states]

    async def run(self, emit: Emit) -> None:
        while True:
            for update in self.tick():
                await emit(update)
            await asyncio.sleep(self._interval)
