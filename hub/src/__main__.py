from __future__ import annotations

import logging
import os

import uvicorn


def main() -> None:
    logging.basicConfig(
        level=os.environ.get("TAMA_LOG_LEVEL", "INFO"),
        format="%(asctime)s %(levelname)s %(name)s %(message)s",
    )
    uvicorn.run(
        "src.app:app",
        host=os.environ.get("TAMA_HUB_HOST", "0.0.0.0"),
        port=int(os.environ.get("TAMA_HUB_PORT", "8000")),
        log_level=os.environ.get("TAMA_LOG_LEVEL", "info").lower(),
    )


if __name__ == "__main__":
    main()
