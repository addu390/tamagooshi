from __future__ import annotations

import logging
import os

import uvicorn

from src.api import lifecycle
from src.app import create_app


def _serve_once() -> None:
    server = uvicorn.Server(uvicorn.Config(
        create_app(),
        host=os.environ.get("TAMA_HUB_HOST", "0.0.0.0"),
        port=int(os.environ.get("TAMA_HUB_PORT", "8000")),
        log_level=os.environ.get("TAMA_LOG_LEVEL", "info").lower(),
        timeout_graceful_shutdown=int(os.environ.get("TAMA_HUB_SHUTDOWN_TIMEOUT", "3")),
    ))

    def stop() -> None:
        server.should_exit = True

    lifecycle.restart.bind(stop)
    server.run()


def main() -> None:
    logging.basicConfig(
        level=os.environ.get("TAMA_LOG_LEVEL", "INFO"),
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )
    while True:
        _serve_once()
        if not lifecycle.restart.consume():
            break
        logging.getLogger("tamagooshi.app").info("restarting to apply changes")


if __name__ == "__main__":
    main()
