from __future__ import annotations

import abc
from collections.abc import Callable
from dataclasses import dataclass

MessageHandler = Callable[[str, str], None]
LineHandler = Callable[[str], None]
StateHandler = Callable[["LinkStatus"], None]


@dataclass(frozen=True)
class LinkStatus:
    state: str
    device: dict | None = None


class Transport(abc.ABC):
    _state: str = "connecting"
    _state_handler: StateHandler | None = None

    @abc.abstractmethod
    def on_message(self, handler: MessageHandler) -> None: ...

    @abc.abstractmethod
    def publish(self, topic: str, payload: bytes, qos: int = 1, retain: bool = False) -> None: ...

    @abc.abstractmethod
    def connect(self) -> None: ...

    @abc.abstractmethod
    def close(self) -> None: ...

    def on_state(self, handler: StateHandler) -> None:
        self._state_handler = handler

    def status(self) -> LinkStatus:
        return LinkStatus(state=self._state, device=self.device_info())

    def device_info(self) -> dict | None:
        return None

    def _set_state(self, state: str) -> None:
        if state == self._state:
            return

        self._state = state
        if self._state_handler is not None:
            self._state_handler(self.status())


class LineChannel(abc.ABC):
    """Newline-delimited JSON side channel carried on the same link as the transport."""

    @abc.abstractmethod
    def on_line(self, handler: LineHandler) -> None: ...

    @abc.abstractmethod
    def send_line(self, line: str) -> None: ...
