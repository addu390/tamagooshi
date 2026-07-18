from __future__ import annotations

import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI

from .config import HubConfig, load_config
from .messaging import InboundRegistry, InboundRouter, Publisher
from .messaging.transport import create_transport
from .pipeline import Pipeline
from .worker import Worker

log = logging.getLogger("tamagooshi.app")


def create_app(config: HubConfig | None = None) -> FastAPI:
    config = config or load_config()

    @asynccontextmanager
    async def lifespan(app: FastAPI):
        inbound = InboundRegistry()
        router = InboundRouter(inbound)
        transport = create_transport(config)
        publisher = Publisher(transport, config.device_id)
        pipeline = Pipeline(config, publisher)
        worker = Worker(config, pipeline)

        router.on("device.hello", lambda _topic, _env: pipeline.replay())
        router.on("page.ack", lambda _topic, env: pipeline.acknowledge(env))

        publisher.on_inbound(router.handle)
        publisher.connect()
        await worker.start()

        app.state.config = config
        app.state.inbound = inbound
        app.state.publisher = publisher
        app.state.pipeline = pipeline
        app.state.worker = worker
        try:
            yield
        finally:
            await worker.stop()
            publisher.close()

    app = FastAPI(title="Tamagooshi Hub", version="0.1.0", lifespan=lifespan)

    @app.get("/healthz")
    async def healthz():
        return {"status": "ok", "device_id": config.device_id}

    @app.get("/config")
    async def get_config():
        return config.model_dump()

    @app.get("/devices")
    async def devices():
        return app.state.inbound.snapshot()

    @app.get("/alerts")
    async def alerts():
        pipeline = app.state.pipeline
        return {
            "mood": pipeline.current_mood,
            "metrics": pipeline.metric_snapshot(),
            "alerts": pipeline.alerts.snapshot(),
        }

    return app


app = create_app()
