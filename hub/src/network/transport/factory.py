from __future__ import annotations

import os
from typing import Callable

from ...config import HubConfig
from .base import Transport
from .mqtt import MqttTransport

TransportFactory = Callable[[HubConfig], Transport]


def _ble(config: HubConfig) -> Transport:
    from .ble import BleTransport

    return BleTransport(name=os.getenv("TAMA_BLE_NAME"), address=os.getenv("TAMA_BLE_ADDRESS"))


def _mqtt(config: HubConfig) -> Transport:
    return MqttTransport(config.broker.host, config.broker.port, config.device_id)


TRANSPORTS: dict[str, dict[str, TransportFactory]] = {
    "ble": {"gatt": _ble},
    "wifi": {"mqtt": _mqtt},
}


def create_transport(config: HubConfig) -> Transport:
    link, _, protocol = os.getenv("TAMA_TRANSPORT", "ble").lower().partition(":")
    protocols = TRANSPORTS.get(link)
    if protocols is None:
        raise ValueError(f"unknown transport link: {link}")
    protocol = protocol or next(iter(protocols))
    factory = protocols.get(protocol)
    if factory is None:
        raise ValueError(f"link '{link}' does not speak protocol '{protocol}'")
    return factory(config)
