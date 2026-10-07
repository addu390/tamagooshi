from __future__ import annotations

import asyncio
import logging
from contextlib import asynccontextmanager
from dataclasses import asdict

from fastapi import FastAPI

from .api import (
    brands_router,
    config_router,
    connection_router,
    events_router,
    flash_router,
    mount_ui,
    rules_router,
    status_router,
)
from .config import BrandService, HubConfig, default_catalog, load_config
from .config.settings import load_connection
from .network import InboundRegistry, InboundRouter, Publisher
from .network.transport import create_transport
from .services.events import EventBus
from .services.flash import Flasher
from .services.sync import DeviceSync

log = logging.getLogger("tamagooshi.app")


def _publish_saved_hid_mode(publisher: Publisher) -> None:
    mode = load_connection().get("hid_mode")
    if mode:
        publisher.publish_hid_mode(mode)


def _on_device_hello(publisher: Publisher, sync: DeviceSync) -> None:
    _publish_saved_hid_mode(publisher)
    sync.replay()


def create_app(config: HubConfig | None = None) -> FastAPI:
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
        sync = DeviceSync(config, publisher)

        router.on("device.hello", lambda _topic, _env: _on_device_hello(publisher, sync))
        publisher.on_inbound(router.handle)

        publisher.connect()
        sync.announce()
        _publish_saved_hid_mode(publisher)

        app.state.config = config
        app.state.bus = bus
        app.state.transport = transport
        app.state.inbound = inbound
        app.state.publisher = publisher
        app.state.flasher = Flasher(lambda data: bus.publish("flash", data))
        try:
            yield
        finally:
            publisher.close()

    app = FastAPI(title="Tamagooshi Hub", version="0.1.0", lifespan=lifespan)
    app.state.config = config
    app.state.brands = BrandService(catalog)

    app.include_router(status_router)
    app.include_router(events_router)
    app.include_router(brands_router)
    app.include_router(rules_router)
    app.include_router(config_router)
    app.include_router(connection_router)
    app.include_router(flash_router)
    mount_ui(app)
    return app


app = create_app()
