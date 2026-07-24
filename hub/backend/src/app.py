from __future__ import annotations

import asyncio
import logging
from contextlib import asynccontextmanager
from dataclasses import asdict

from fastapi import FastAPI

from .api import (
    agents_router,
    brands_router,
    config_router,
    connection_router,
    events_router,
    flash_router,
    mount_ui,
    rules_router,
    secrets_router,
    sources_router,
    status_router,
)
from .config import BrandService, HubConfig, default_catalog, load_config
from .config.secrets import apply_secrets
from .features.buddy import create_bridge
from .network import InboundRegistry, InboundRouter, Publisher
from .network.transport import create_transport
from .services.events import EventBus
from .services.flash import Flasher
from .services.metrics import Pipeline
from .services.worker import Worker

log = logging.getLogger("tamagooshi.app")


def create_app(config: HubConfig | None = None) -> FastAPI:
    apply_secrets()
    catalog = default_catalog()
    config = config or load_config(catalog)

    @asynccontextmanager
    async def lifespan(app: FastAPI):
        bus = EventBus()
        bus.attach(asyncio.get_running_loop())

        inbound = InboundRegistry(
            observer=lambda info: bus.publish("device", {"device_id": info.device_id})
        )
        router = InboundRouter(inbound)
        transport = create_transport(config)
        transport.on_state(lambda status: bus.publish("link", asdict(status)))
        publisher = Publisher(transport, config.device_id)
        pipeline = Pipeline(config, publisher, bus)
        worker = Worker(config, pipeline)

        router.on("device.hello", lambda _topic, _env: pipeline.replay())
        router.on("page.ack", lambda _topic, env: pipeline.acknowledge(env))
        publisher.on_inbound(router.handle)

        bridge = create_bridge(transport, config.agent)
        if bridge is not None:
            bridge.start()

        publisher.connect()
        await worker.start()

        app.state.config = config
        app.state.bus = bus
        app.state.transport = transport
        app.state.inbound = inbound
        app.state.publisher = publisher
        app.state.pipeline = pipeline
        app.state.worker = worker
        app.state.bridge = bridge
        app.state.flasher = Flasher(lambda data: bus.publish("flash", data))
        try:
            yield
        finally:
            await worker.stop()
            if bridge is not None:
                await bridge.stop()
            publisher.close()

    app = FastAPI(title="Tamagooshi Hub", version="0.1.0", lifespan=lifespan)
    app.state.config = config
    app.state.brands = BrandService(catalog)

    app.include_router(status_router)
    app.include_router(events_router)
    app.include_router(agents_router)
    app.include_router(brands_router)
    app.include_router(sources_router)
    app.include_router(secrets_router)
    app.include_router(rules_router)
    app.include_router(config_router)
    app.include_router(connection_router)
    app.include_router(flash_router)
    mount_ui(app)
    return app


app = create_app()
