from __future__ import annotations

from .conditions import matches
from .rules import MoodRule
from .store import MetricStore


class MoodEngine:
    def __init__(self, rules: list[MoodRule], default: str):
        self._rules = sorted(rules, key=lambda r: r.priority, reverse=True)
        self._default = default

    def evaluate(self, store: MetricStore) -> str:
        for rule in self._rules:
            if matches(rule.when, store):
                return rule.mood
        return self._default
