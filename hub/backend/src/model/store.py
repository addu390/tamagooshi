from __future__ import annotations

from .types import MetricUpdate


class MetricStore:
    def __init__(self):
        self._raw: dict[str, float] = {}
        self._last: dict[str, MetricUpdate] = {}

    def update(self, update: MetricUpdate) -> None:
        self._last[update.key] = update
        if update.raw is not None:
            self._raw[update.key] = update.raw

    def value(self, key: str) -> float | None:
        return self._raw.get(key)

    def snapshot(self) -> dict[str, float]:
        return dict(self._raw)

    def last(self) -> list[MetricUpdate]:
        return list(self._last.values())
