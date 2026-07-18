from __future__ import annotations

import asyncio
import logging
import threading
from typing import Optional

from .base import AgentBackend, AgentRunError, AgentStartupError, OnDelta

log = logging.getLogger("tamagooshi.buddy.claude")


class ClaudeBackend(AgentBackend):
    """Multi-turn Claude agent via the claude-agent-sdk, kept open across prompts.

    The SDK is async-only, so the backend owns a private event loop on a daemon
    thread and bridges the blocking AgentBackend contract onto it.
    """

    def __init__(self, cwd: str, model: Optional[str] = None):
        self._cwd = cwd
        self._model = model
        self._client = None
        self._lock = threading.Lock()
        self._loop: Optional[asyncio.AbstractEventLoop] = None
        self._thread: Optional[threading.Thread] = None

    def send(self, text: str, on_delta: OnDelta) -> str:
        with self._lock:
            loop = self._ensure_loop()
            future = asyncio.run_coroutine_threadsafe(self._send(text, on_delta), loop)
            return future.result()

    def close(self) -> None:
        with self._lock:
            if self._loop is None:
                return
            loop, self._loop = self._loop, None
            asyncio.run_coroutine_threadsafe(self._disconnect(), loop).result(timeout=10)
            loop.call_soon_threadsafe(loop.stop)
            if self._thread is not None:
                self._thread.join(timeout=10)
                self._thread = None

    def _ensure_loop(self) -> asyncio.AbstractEventLoop:
        if self._loop is None:
            loop = asyncio.new_event_loop()
            thread = threading.Thread(target=loop.run_forever, name="claude-agent", daemon=True)
            thread.start()
            self._loop = loop
            self._thread = thread
        return self._loop

    async def _send(self, text: str, on_delta: OnDelta) -> str:
        from claude_agent_sdk import AssistantMessage, CLINotFoundError, TextBlock

        try:
            client = await self._ensure_client()
            await client.query(text)
        except CLINotFoundError as err:
            self._client = None
            raise AgentStartupError(str(err)) from err
        except Exception as err:  # noqa: BLE001 - SDK raises on spawn/auth problems
            self._client = None
            raise AgentStartupError(str(err)) from err

        parts: list[str] = []
        try:
            async for message in client.receive_response():
                if isinstance(message, AssistantMessage):
                    for block in message.content:
                        if isinstance(block, TextBlock):
                            parts.append(block.text)
                            on_delta(block.text)
        except Exception as err:  # noqa: BLE001
            raise AgentRunError(str(err)) from err
        return "".join(parts)

    async def _ensure_client(self):
        if self._client is None:
            from claude_agent_sdk import ClaudeAgentOptions, ClaudeSDKClient

            options = ClaudeAgentOptions(cwd=self._cwd, model=self._model)
            client = ClaudeSDKClient(options=options)
            await client.connect()
            self._client = client
            log.info("connected claude agent (cwd=%s, model=%s)", self._cwd, self._model or "default")
        return self._client

    async def _disconnect(self) -> None:
        if self._client is not None:
            try:
                await self._client.disconnect()
            finally:
                self._client = None
