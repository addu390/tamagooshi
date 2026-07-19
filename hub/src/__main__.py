from __future__ import annotations

import logging
import os
import signal
import threading
import time

import uvicorn

from src.app import create_app


def _watch_parent() -> None:
    parent = os.getppid()
    while os.getppid() == parent:
        time.sleep(2)
    os.kill(os.getpid(), signal.SIGTERM)


def main() -> None:
    logging.basicConfig(
        level=os.environ.get("TAMA_LOG_LEVEL", "INFO"),
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )
    if os.environ.get("TAMA_PARENT_WATCH") == "1":
        threading.Thread(target=_watch_parent, daemon=True).start()
    uvicorn.run(
        create_app(),
        host=os.environ.get("TAMA_HUB_HOST", "0.0.0.0"),
        port=int(os.environ.get("TAMA_HUB_PORT", "8000")),
        log_level=os.environ.get("TAMA_LOG_LEVEL", "info").lower(),
    )


if __name__ == "__main__":
    main()
