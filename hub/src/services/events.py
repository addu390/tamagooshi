from __future__ import annotations

import asyncio
import time
from dataclasses import dataclass, field
from typing import AsyncIterator, List, Optional


@dataclass(frozen=True)
class Event:
    type: str
    data: dict
    ts: float = field(default_factory=time.time)


class EventBus:
    def __init__(self, history: int = 50):
        self._loop: Optional[asyncio.AbstractEventLoop] = None
        self._queues: List[asyncio.Queue[Event]] = []
        self._history: List[Event] = []
        self._max_history = history

    def attach(self, loop: asyncio.AbstractEventLoop) -> None:
        self._loop = loop

    def publish(self, type_: str, data: dict) -> None:
        event = Event(type=type_, data=data)
        self._history.append(event)
        del self._history[:-self._max_history]

        if self._loop is None:
            return
        self._loop.call_soon_threadsafe(self._fanout, event)

    def _fanout(self, event: Event) -> None:
        for queue in self._queues:
            queue.put_nowait(event)

    async def listen(self, replay: bool = False) -> AsyncIterator[Event]:
        queue: asyncio.Queue[Event] = asyncio.Queue()
        if replay:
            for event in self._history:
                queue.put_nowait(event)

        self._queues.append(queue)
        try:
            while True:
                yield await queue.get()
        finally:
            self._queues.remove(queue)
