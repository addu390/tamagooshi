from __future__ import annotations

import time as _time

from ..config import BrandConfig
from ..wire import protocol, topics
from .transport import MessageHandler, Transport


class Publisher:
    def __init__(self, transport: Transport, device_id: str):
        self._transport = transport
        self._device_id = device_id

    def on_inbound(self, handler: MessageHandler) -> None:
        self._transport.on_message(handler)

    def connect(self) -> None:
        self._transport.connect()

    def close(self) -> None:
        self._transport.close()

    def publish_envelope(self, topic: str, type_: str, body: protocol.BaseModel) -> None:
        self._transport.publish(topic, protocol.envelope(type_, body).encode("utf-8"))

    def publish_branding(self, brand: BrandConfig) -> None:
        body = protocol.Branding(name=brand.name, tagline=brand.tagline, logo_id=brand.logo_id)
        self.publish_envelope(topics.branding(self._device_id), "branding.set", body)

    def publish_config(self, brand: BrandConfig) -> None:
        body = protocol.ConfigSet(
            theme=brand.theme, character_id=brand.mascot, carousel_secs=brand.carousel_secs
        )
        self.publish_envelope(topics.config(self._device_id), "config.set", body)

    def publish_hid_mode(self, mode: str) -> None:
        body = protocol.ConfigSet(hid_mode=mode)
        self.publish_envelope(topics.config(self._device_id), "config.set", body)

    def publish_time(self, tz_offset: int) -> None:
        body = protocol.TimeSet(epoch=int(_time.time()), tz_offset=tz_offset)
        self.publish_envelope(topics.time(self._device_id), "time.set", body)
