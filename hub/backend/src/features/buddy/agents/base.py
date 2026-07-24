from __future__ import annotations

import abc
from collections.abc import Callable

OnDelta = Callable[[str], None]


class AgentStartupError(RuntimeError):
    """The agent never executed: auth, config, or connectivity. Fix the environment."""


class AgentRunError(RuntimeError):
    """The agent ran and failed mid-flight."""


class AgentBackend(abc.ABC):
    """A conversational coding agent. Calls are blocking; run them off the event loop."""

    @abc.abstractmethod
    def send(self, text: str, on_delta: OnDelta) -> str:
        """Sends one prompt on the ongoing conversation, streaming deltas, returning the reply."""

    @abc.abstractmethod
    def close(self) -> None: ...
