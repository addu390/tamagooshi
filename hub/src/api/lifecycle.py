from __future__ import annotations

import asyncio
import os
import signal

from fastapi import HTTPException
from pydantic import ValidationError


def schedule_restart(delay: float = 0.5) -> None:
    loop = asyncio.get_running_loop()
    loop.call_later(delay, os.kill, os.getpid(), signal.SIGTERM)


def apply_change(mutation) -> dict:
    try:
        mutation()
    except (ValidationError, ValueError) as err:
        raise HTTPException(status_code=400, detail=str(err)) from err

    schedule_restart()
    return {"restarting": True}
