from __future__ import annotations

import threading
from dataclasses import dataclass, field

from .conditions import matches
from .rules import AlertRule
from .store import MetricStore


@dataclass
class AlertTransition:
    raised: list[AlertRule] = field(default_factory=list)
    cleared: list[str] = field(default_factory=list)


class AlertEngine:
    def __init__(self, rules: list[AlertRule]):
        self._rules = {r.id: r for r in rules}
        self._active: set[str] = set()
        self._acked: set[str] = set()
        self._lock = threading.Lock()

    def evaluate(self, store: MetricStore) -> AlertTransition:
        with self._lock:
            now_active = {rid for rid, r in self._rules.items() if matches(r.when, store)}
            raised = [self._rules[rid] for rid in self._rules if rid in now_active and rid not in self._active]
            cleared = [rid for rid in self._active if rid not in now_active]
            self._active = now_active
            for rid in cleared:
                self._acked.discard(rid)
            return AlertTransition(raised=raised, cleared=cleared)

    def acknowledge(self, page_id: str) -> None:
        with self._lock:
            if page_id in self._active:
                self._acked.add(page_id)

    def active_rules(self) -> list[AlertRule]:
        with self._lock:
            return [self._rules[rid] for rid in self._active]

    def snapshot(self) -> dict[str, object]:
        with self._lock:
            return {
                "active": sorted(self._active),
                "acknowledged": sorted(self._acked),
            }
