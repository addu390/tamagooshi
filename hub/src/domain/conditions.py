from __future__ import annotations

import operator

from .rules import Condition
from .store import MetricStore

_OPS = {
    "lt": operator.lt,
    "lte": operator.le,
    "gt": operator.gt,
    "gte": operator.ge,
    "eq": operator.eq,
    "ne": operator.ne,
}


def matches(cond: Condition, store: MetricStore) -> bool:
    v = store.value(cond.metric)
    if v is None:
        return False
    return _OPS[cond.op](v, cond.value)
