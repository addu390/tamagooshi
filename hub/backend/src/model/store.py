from __future__ import annotations

from typing import Dict, List, Optional

from .types import MetricUpdate


class MetricStore:
    def __init__(self):
        self._raw: Dict[str, float] = {}
        self._last: Dict[str, MetricUpdate] = {}

    def update(self, update: MetricUpdate) -> None:
        self._last[update.key] = update
        if update.raw is not None:
            self._raw[update.key] = update.raw

    def value(self, key: str) -> Optional[float]:
        return self._raw.get(key)

    def snapshot(self) -> Dict[str, float]:
        return dict(self._raw)

    def last(self) -> List[MetricUpdate]:
        return list(self._last.values())
