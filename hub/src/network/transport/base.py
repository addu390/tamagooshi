from __future__ import annotations

import abc
from typing import Callable

MessageHandler = Callable[[str, str], None]
LineHandler = Callable[[str], None]


class Transport(abc.ABC):
    @abc.abstractmethod
    def on_message(self, handler: MessageHandler) -> None: ...

    @abc.abstractmethod
    def publish(self, topic: str, payload: bytes, qos: int = 1, retain: bool = False) -> None: ...

    @abc.abstractmethod
    def connect(self) -> None: ...

    @abc.abstractmethod
    def close(self) -> None: ...


class LineChannel(abc.ABC):
    """Newline-delimited JSON side channel carried on the same link as the transport."""

    @abc.abstractmethod
    def on_line(self, handler: LineHandler) -> None: ...

    @abc.abstractmethod
    def send_line(self, line: str) -> None: ...
