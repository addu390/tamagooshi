from __future__ import annotations

import os
from collections.abc import Callable

from .base import AgentBackend
from .catalog import AGENTS


def _make_cursor() -> AgentBackend:
    from .cursor import CursorBackend

    return CursorBackend(
        cwd=os.getenv("TAMA_AGENT_CWD", os.getcwd()),
        model=os.getenv("TAMA_CURSOR_MODEL", "composer-2.5"),
    )


def _make_claude() -> AgentBackend:
    from .claude import ClaudeBackend

    return ClaudeBackend(
        cwd=os.getenv("TAMA_AGENT_CWD", os.getcwd()),
        model=os.getenv("TAMA_CLAUDE_MODEL"),
    )


_FACTORIES: dict[str, Callable[[], AgentBackend]] = {
    "cursor": _make_cursor,
    "claude": _make_claude,
}


def known_agents() -> list[str]:
    return sorted(AGENTS)


def create_agent(agent_id: str) -> AgentBackend:
    try:
        factory = _FACTORIES[agent_id]
    except KeyError:
        known = ", ".join(known_agents())
        raise ValueError(f"unknown agent backend: {agent_id!r} (known: {known})") from None
    return factory()
