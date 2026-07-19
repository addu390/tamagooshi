from __future__ import annotations

import logging
import time
from dataclasses import dataclass, field
from typing import Callable, Dict

from ..wire import protocol

log = logging.getLogger("tamagooshi.inbound")

EnvelopeHandler = Callable[[str, protocol.Envelope], None]


@dataclass
class DeviceInfo:
    device_id: str
    last_topic: str = ""
    last_seen: float = 0.0
    last_body: dict = field(default_factory=dict)


DeviceObserver = Callable[[DeviceInfo], None]


class InboundRegistry:
    def __init__(self, observer: DeviceObserver | None = None):
        self._devices: Dict[str, DeviceInfo] = {}
        self._observer = observer

    def track(self, topic: str, body: dict) -> None:
        parts = topic.split("/")
        device_id = parts[1] if len(parts) > 1 else "?"
        fresh = device_id not in self._devices

        info = self._devices.setdefault(device_id, DeviceInfo(device_id=device_id))
        info.last_topic = topic
        info.last_seen = time.time()
        info.last_body = body

        if fresh and self._observer is not None:
            self._observer(info)

    def snapshot(self) -> Dict[str, dict]:
        return {
            dev_id: {
                "device_id": info.device_id,
                "last_topic": info.last_topic,
                "last_seen": info.last_seen,
                "last_body": info.last_body,
            }
            for dev_id, info in self._devices.items()
        }


class InboundRouter:
    def __init__(self, registry: InboundRegistry):
        self._registry = registry
        self._handlers: Dict[str, EnvelopeHandler] = {}

    def on(self, type_: str, handler: EnvelopeHandler) -> None:
        self._handlers[type_] = handler

    def handle(self, topic: str, payload: str) -> None:
        env = protocol.parse_envelope(payload)
        if env is None:
            log.warning("unparseable inbound on %s", topic)
            self._registry.track(topic, {})
            return

        log.info("inbound %s: %s", topic, env.type)
        self._registry.track(topic, env.body)

        handler = self._handlers.get(env.type)
        if handler:
            handler(topic, env)
