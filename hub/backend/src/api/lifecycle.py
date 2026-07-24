from __future__ import annotations

import asyncio
import os
import signal
from collections.abc import Callable

from fastapi import HTTPException
from pydantic import ValidationError


class RestartController:
    def __init__(self) -> None:
        self._pending = False
        self._stop: Callable[[], None] | None = None

    def bind(self, stop: Callable[[], None]) -> None:
        self._stop = stop

    def consume(self) -> bool:
        pending = self._pending
        self._pending = False
        return pending

    def schedule(self, delay: float = 0.5) -> None:
        self._pending = True
        loop = asyncio.get_running_loop()
        if self._stop is not None:
            loop.call_later(delay, self._stop)
        else:
            loop.call_later(delay, os.kill, os.getpid(), signal.SIGTERM)


restart = RestartController()


def schedule_restart(delay: float = 0.5) -> None:
    restart.schedule(delay)


def apply_change(mutation) -> dict:
    try:
        mutation()
    except (ValidationError, ValueError) as err:
        raise HTTPException(status_code=400, detail=str(err)) from err

    schedule_restart()
    return {"restarting": True}
