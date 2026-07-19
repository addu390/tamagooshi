from __future__ import annotations

import asyncio
from typing import List

from ...model import MetricUpdate
from .base import Source


class CollectError(Exception):
    pass


async def collect_once(source: Source, timeout: float = 30) -> List[MetricUpdate]:
    collected: List[MetricUpdate] = []

    async def emit(update: MetricUpdate) -> None:
        collected.append(update)

    try:
        await asyncio.wait_for(source.poll_once(emit), timeout=timeout)
    except asyncio.TimeoutError as err:
        raise CollectError(f"test timed out after {timeout:.0f}s") from err
    except Exception as err:  # noqa: BLE001 - normalized for callers to report
        raise CollectError(str(err)) from err
    return collected
