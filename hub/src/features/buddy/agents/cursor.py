from __future__ import annotations

import logging
import os
import threading

from .base import AgentBackend, AgentRunError, AgentStartupError, OnDelta

log = logging.getLogger("tamagooshi.buddy.cursor")


class CursorBackend(AgentBackend):
    """Multi-turn Cursor agent via the cursor-sdk, kept open across prompts."""

    def __init__(self, cwd: str, model: str = "composer-2.5", api_key: str | None = None):
        self._cwd = cwd
        self._model = model
        self._api_key = api_key or os.environ.get("CURSOR_API_KEY")
        self._agent = None
        self._lock = threading.Lock()

    def send(self, text: str, on_delta: OnDelta) -> str:
        from cursor_sdk import CursorAgentError

        with self._lock:
            try:
                agent = self._ensure_agent()
                run = agent.send(text)
                log.info("cursor run %s on agent %s", run.id, agent.agent_id)
            except CursorAgentError as err:
                self._agent = None
                raise AgentStartupError(str(err)) from err

            parts: list[str] = []
            try:
                for chunk in run.iter_text():
                    parts.append(chunk)
                    on_delta(chunk)
            finally:
                result = run.wait()

            if result.status == "error":
                raise AgentRunError(f"cursor run {result.id} failed")
            return "".join(parts)

    def close(self) -> None:
        with self._lock:
            if self._agent is not None:
                try:
                    self._agent.close()
                finally:
                    self._agent = None

    def _ensure_agent(self):
        if self._agent is None:
            from cursor_sdk import Agent, LocalAgentOptions

            if not self._api_key:
                raise AgentStartupError("CURSOR_API_KEY is not set")
            self._agent = Agent.create(
                model=self._model,
                api_key=self._api_key,
                local=LocalAgentOptions(cwd=self._cwd),
            )
            log.info("created cursor agent %s (cwd=%s)", self._agent.agent_id, self._cwd)
        return self._agent
