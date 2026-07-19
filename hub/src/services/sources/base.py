from __future__ import annotations

import abc
from typing import Awaitable, Callable

from ...model import MetricUpdate

Emit = Callable[[MetricUpdate], Awaitable[None]]


class Source(abc.ABC):
    @abc.abstractmethod
    async def run(self, emit: Emit) -> None:
        raise NotImplementedError

    @abc.abstractmethod
    async def poll_once(self, emit: Emit) -> None:
        raise NotImplementedError

    def metric_status(self) -> dict[str, dict]:
        return {}
