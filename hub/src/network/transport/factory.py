from __future__ import annotations

import os
from typing import Callable

from ...config import HubConfig
from ...config.settings import load_connection
from .base import Transport
from .mqtt import MqttTransport

TransportFactory = Callable[[HubConfig], Transport]


def _ble(config: HubConfig) -> Transport:
    from .ble import BleTransport

    saved = load_connection().get("device") or {}
    return BleTransport(name=os.getenv("TAMA_BLE_NAME") or saved.get("name"),
                        address=os.getenv("TAMA_BLE_ADDRESS") or saved.get("address"))


def _mqtt(config: HubConfig) -> Transport:
    return MqttTransport(config.broker.host, config.broker.port, config.device_id)


# Keys mirror firmware/tools/gen/network/transports.py
TRANSPORTS: dict[str, dict[str, TransportFactory]] = {
    "ble": {"gatt": _ble},
    "wifi": {"mqtt": _mqtt},
}


def transport_spec() -> str:
    return (os.getenv("TAMA_TRANSPORT") or load_connection().get("transport") or "ble").lower()


def spec_locked() -> bool:
    return bool(os.getenv("TAMA_TRANSPORT"))


def resolve_spec(spec: str) -> tuple[str, str, TransportFactory]:
    link, _, protocol = spec.lower().partition(":")

    protocols = TRANSPORTS.get(link)
    if protocols is None:
        raise ValueError(f"unknown transport link: {link}")

    protocol = protocol or next(iter(protocols))
    factory = protocols.get(protocol)
    if factory is None:
        raise ValueError(f"link '{link}' does not speak protocol '{protocol}'")

    return link, protocol, factory


def create_transport(config: HubConfig) -> Transport:
    _, _, factory = resolve_spec(transport_spec())
    return factory(config)
