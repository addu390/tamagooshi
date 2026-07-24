from __future__ import annotations

import asyncio
import json
import logging
import threading
from collections.abc import Callable

from ...network.transport.base import LineChannel
from .agents.base import AgentBackend, AgentRunError, AgentStartupError
from .transcriber import Transcriber
from .voice import SAMPLE_RATE, VoiceAssembler

log = logging.getLogger("tamagooshi.buddy.bridge")

FLUSH_INTERVAL = 0.8

BackendFactory = Callable[[str], AgentBackend]


class VoiceBridge:
    """Turns device voice uploads into agent prompts and streams the reply back.

    Speaks the NUS NDJSON line protocol: consumes agents/voice/voice_end/permission
    commands, produces agents/transcript/reply events plus buddy snapshots for the
    device mood. The device picks the agent per ask; backends are created lazily
    and kept alive for follow-up turns.
    """

    def __init__(self, channel: LineChannel, transcriber: Transcriber,
                 create_backend: BackendFactory, enabled: list[str], default: str):
        self._channel = channel
        self._transcriber = transcriber
        self._create_backend = create_backend
        self._enabled = enabled
        self._default = default
        self._backends: dict[str, AgentBackend] = {}
        self._assembler = VoiceAssembler()
        self._loop: asyncio.AbstractEventLoop | None = None
        self._submit_lock = asyncio.Lock()
        self._pending: dict[str, tuple[str, str]] = {}
        self._seq = 0

    def start(self) -> None:
        self._loop = asyncio.get_running_loop()
        self._channel.on_line(self._on_line)
        self._send_roster()
        log.info("voice bridge attached (agents: %s, default: %s)",
                 ", ".join(self._enabled), self._default)

    async def stop(self) -> None:
        for backend in self._backends.values():
            await asyncio.to_thread(backend.close)
        self._backends.clear()

    def _on_line(self, line: str) -> None:
        assert self._loop is not None
        asyncio.run_coroutine_threadsafe(self._handle(line), self._loop)

    async def _handle(self, line: str) -> None:
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            log.debug("ignoring non-json line: %.80s", line)
            return

        cmd = msg.get("cmd")
        if cmd == "agents":
            self._send_roster()
        elif cmd == "voice":
            self._assembler.add_chunk(int(msg.get("seq", -1)), msg.get("data", ""))
        elif cmd == "voice_end":
            await self._transcribe(self._resolve_agent(msg.get("agent")))
        elif cmd == "permission":
            await self._decide(str(msg.get("id", "")), msg.get("decision") == "once")

    def _resolve_agent(self, requested: object) -> str:
        if isinstance(requested, str) and requested in self._enabled:
            return requested
        if requested:
            log.warning("device asked for unknown agent %r; using %s", requested, self._default)
        return self._default

    def _backend(self, agent: str) -> AgentBackend:
        backend = self._backends.get(agent)
        if backend is None:
            backend = self._create_backend(agent)
            self._backends[agent] = backend
        return backend

    async def _transcribe(self, agent: str) -> None:
        pcm = self._assembler.finish()
        if pcm is None:
            self._send_reply("I lost part of that recording, please try again.", done=True)
            return

        try:
            text = await asyncio.to_thread(self._transcriber.transcribe, pcm, SAMPLE_RATE)
        except Exception:
            log.exception("transcription failed")
            self._send_reply("Transcription failed on the hub, check its logs.", done=True)
            return
        if not text:
            self._send_reply("I didn't catch any speech in that, please try again.", done=True)
            return

        self._seq += 1
        voice_id = f"v{self._seq}"
        self._pending[voice_id] = (text, agent)

        log.info("transcript %s (%s): %s", voice_id, agent, text)
        self._send({"evt": "transcript", "id": voice_id, "text": text, "agent": agent})

    async def _decide(self, voice_id: str, approved: bool) -> None:
        pending = self._pending.pop(voice_id, None)
        if pending is None:
            return

        text, agent = pending
        if not approved:
            log.info("prompt %s cancelled on device", voice_id)
            return

        async with self._submit_lock:
            await self._submit(text, agent)

    async def _submit(self, text: str, agent: str) -> None:
        self._send_snapshot(running=1, msg=f"asking {agent}")

        buffer: list[str] = []
        buffer_lock = threading.Lock()

        def on_delta(chunk: str) -> None:
            with buffer_lock:
                buffer.append(chunk)

        def flush(done: bool) -> None:
            with buffer_lock:
                pending = "".join(buffer)
                buffer.clear()
            if pending or done:
                self._send_reply(pending, done=done)

        try:
            backend = self._backend(agent)
        except Exception:
            log.exception("failed to create %s backend", agent)
            self._send_reply(f"The {agent} backend could not be created, check the hub logs.",
                             done=True)
            self._send_snapshot(running=0, msg="agent error")
            return

        task = asyncio.create_task(asyncio.to_thread(backend.send, text, on_delta))
        try:
            while not task.done():
                await asyncio.wait({task}, timeout=FLUSH_INTERVAL)
                if not task.done():
                    flush(done=False)
            await task
            flush(done=True)
            self._send_snapshot(running=0, msg="done")
        except AgentStartupError as err:
            log.error("%s agent failed to start: %s", agent, err)
            flush(done=False)
            self._send_reply(f"The {agent} agent failed to start: {err}", done=True)
            self._send_snapshot(running=0, msg="agent error")
        except AgentRunError as err:
            log.error("%s run failed: %s", agent, err)
            flush(done=False)
            self._send_reply(f"The run failed: {err}", done=True)
            self._send_snapshot(running=0, msg="run failed")
        except Exception:
            log.exception("agent backend crashed")
            flush(done=False)
            self._send_reply("The agent crashed on the hub, check its logs.", done=True)
            self._send_snapshot(running=0, msg="agent error")

    def _send_roster(self) -> None:
        self._send({"evt": "agents", "enabled": self._enabled, "default": self._default})

    def _send_reply(self, text: str, done: bool) -> None:
        self._send({"evt": "reply", "text": text, "done": done})

    def _send_snapshot(self, running: int, msg: str) -> None:
        self._send({"total": 1, "running": running, "waiting": 0, "msg": msg})

    def _send(self, msg: dict) -> None:
        self._channel.send_line(json.dumps(msg, separators=(",", ":")))
