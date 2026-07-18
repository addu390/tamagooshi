from __future__ import annotations

import abc
from typing import Callable

MessageHandler = Callable[[str, str], None]


class Transport(abc.ABC):
    @abc.abstractmethod
    def on_message(self, handler: MessageHandler) -> None: ...

    @abc.abstractmethod
    def publish(self, topic: str, payload: bytes, qos: int = 1, retain: bool = False) -> None: ...

    @abc.abstractmethod
    def connect(self) -> None: ...

    @abc.abstractmethod
    def close(self) -> None: ...
