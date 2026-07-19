from __future__ import annotations

from fastapi import Request

from ..config.models import HubConfig
from ..network.transport.base import Transport
from ..services.flash import Flasher
from ..services.worker import Worker


def worker(request: Request) -> Worker:
    return request.app.state.worker


def transport(request: Request) -> Transport:
    return request.app.state.transport


def flasher(request: Request) -> Flasher:
    return request.app.state.flasher


def hub_config(request: Request) -> HubConfig:
    return request.app.state.config


def brand_id(request: Request) -> str:
    return request.app.state.config.brand_id
