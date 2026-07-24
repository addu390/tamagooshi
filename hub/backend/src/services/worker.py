from __future__ import annotations

import asyncio
import logging
import time
from dataclasses import dataclass

from ..config import HubConfig
from .metrics import Pipeline
from .sources import Source, SourceConfigBase, build_source

log = logging.getLogger("tamagooshi.worker")


@dataclass
class SourceRuntime:
    index: int
    config: SourceConfigBase
    source: Source | None = None
    task: asyncio.Task | None = None
    error: str | None = None
    last_emit: float | None = None

    @property
    def running(self) -> bool:
        return self.task is not None and not self.task.done()


class Worker:
    def __init__(self, config: HubConfig, pipeline: Pipeline):
        self._pipeline = pipeline
        self._runtimes = [SourceRuntime(i, cfg) for i, cfg in enumerate(config.sources)]

    async def start(self) -> None:
        self._pipeline.announce()

        for runtime in self._runtimes:
            if runtime.config.enabled:
                self._launch(runtime)

        log.info("started %d of %d source(s)",
                 sum(r.running for r in self._runtimes), len(self._runtimes))

    async def stop(self) -> None:
        tasks = [r.task for r in self._runtimes if r.task is not None]
        for task in tasks:
            task.cancel()

        await asyncio.gather(*tasks, return_exceptions=True)
        for runtime in self._runtimes:
            runtime.task = None

    def set_enabled(self, index: int, enabled: bool) -> SourceRuntime:
        runtime = self._runtimes[index]
        runtime.config.enabled = enabled

        if enabled and not runtime.running:
            self._launch(runtime)
        elif not enabled and runtime.task is not None:
            runtime.task.cancel()
            runtime.task = None

        return runtime

    async def refresh(self, index: int) -> SourceRuntime:
        runtime = self._runtimes[index]
        if runtime.source is None:
            runtime.source = self._build(runtime)
        if runtime.source is None:
            return runtime

        try:
            await asyncio.wait_for(runtime.source.poll_once(self._emit_for(runtime)), timeout=30)
            runtime.error = None
        except asyncio.CancelledError:
            raise
        except Exception as exc:  # noqa: BLE001 - surfaced as source status
            runtime.error = str(exc)

        return runtime

    def snapshot(self) -> list[SourceRuntime]:
        return list(self._runtimes)

    def _build(self, runtime: SourceRuntime) -> Source | None:
        try:
            source = build_source(runtime.config)
            runtime.error = None
            return source
        except Exception as exc:  # noqa: BLE001 - surfaced as source status
            log.warning("source %d (%s) unavailable: %s",
                        runtime.index, runtime.config.type, exc)
            runtime.error = str(exc)
            return None

    def _launch(self, runtime: SourceRuntime) -> None:
        runtime.source = self._build(runtime)
        if runtime.source is None:
            return
        runtime.task = asyncio.create_task(self._run(runtime))

    async def _run(self, runtime: SourceRuntime) -> None:
        try:
            await runtime.source.run(self._emit_for(runtime))
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            log.exception("source %d (%s) stopped", runtime.index, runtime.config.type)
            runtime.error = str(exc)

    def _emit_for(self, runtime: SourceRuntime):
        async def emit(update):
            runtime.last_emit = time.time()
            await self._pipeline.emit(update)
        return emit
