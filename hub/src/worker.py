from __future__ import annotations

import asyncio
import logging
from typing import List

from .config import HubConfig
from .pipeline import Pipeline
from .sources import build_source

log = logging.getLogger("tamagooshi.worker")


class Worker:
    def __init__(self, config: HubConfig, pipeline: Pipeline):
        self._config = config
        self._pipeline = pipeline
        self._tasks: List[asyncio.Task] = []

    async def start(self) -> None:
        self._pipeline.announce()
        for source_cfg in self._config.sources:
            source = build_source(source_cfg)
            self._tasks.append(asyncio.create_task(source.run(self._pipeline.emit)))
        log.info("started %d source task(s)", len(self._tasks))

    async def stop(self) -> None:
        for task in self._tasks:
            task.cancel()
        await asyncio.gather(*self._tasks, return_exceptions=True)
        self._tasks.clear()
