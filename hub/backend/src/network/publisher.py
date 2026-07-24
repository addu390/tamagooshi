from __future__ import annotations

import time as _time

from ..config import BrandConfig
from ..model import AlertRule, MetricUpdate
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

    def publish_envelope(self, topic: str, type_: str, body: protocol.BaseModel, retain: bool = True) -> None:
        payload = protocol.envelope(type_, body).encode("utf-8")
        self._transport.publish(topic, payload, qos=1, retain=retain)

    def publish_branding(self, brand: BrandConfig) -> None:
        body = protocol.Branding(name=brand.name, tagline=brand.tagline, logo_id=brand.logo_id)
        self.publish_envelope(topics.branding(self._device_id), "branding.set", body)

    def publish_config(self, brand: BrandConfig) -> None:
        body = protocol.ConfigSet(
            theme=brand.theme, character_id=brand.mascot, carousel_secs=brand.carousel_secs
        )
        self.publish_envelope(topics.config(self._device_id), "config.set", body)

    def publish_time(self, tz_offset: int) -> None:
        body = protocol.TimeSet(epoch=int(_time.time()), tz_offset=tz_offset)
        self.publish_envelope(topics.time(self._device_id), "time.set", body, retain=False)

    def publish_mood(self, state: str, reason: str | None = None) -> None:
        body = protocol.MoodSet(state=state, reason=reason)
        self.publish_envelope(topics.mood(self._device_id), "mood.set", body)

    def publish_metric(self, update: MetricUpdate) -> None:
        body = protocol.MetricUpsert(
            key=update.key,
            label=update.label,
            value=update.value,
            trend=update.trend,
            kind=update.kind,
        )
        self.publish_envelope(topics.metric(self._device_id, update.key), "metric.upsert", body)

    def publish_page(self, rule: AlertRule) -> None:
        body = protocol.PageRaise(
            id=rule.id,
            title=rule.title,
            severity=rule.severity,
            body=rule.body or None,
            source=rule.source,
            requires_ack=rule.requires_ack,
        )
        self.publish_envelope(topics.pages(self._device_id), "page.raise", body, retain=False)

    def clear_page(self, page_id: str) -> None:
        body = protocol.PageRef(id=page_id)
        self.publish_envelope(topics.pages(self._device_id), "page.clear", body, retain=False)
