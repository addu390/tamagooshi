from __future__ import annotations

import logging
from typing import Optional

import paho.mqtt.client as mqtt

from ...wire import topics
from .base import MessageHandler, Transport

log = logging.getLogger("tamagooshi.transport.mqtt")


class MqttTransport(Transport):
    def __init__(self, host: str, port: int, device_id: str, client_id: str = "tamagooshi-hub"):
        self._host = host
        self._port = port
        self._device_id = device_id
        self._handler: Optional[MessageHandler] = None
        self._client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=client_id,
            clean_session=True,
        )
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message

    def on_message(self, handler: MessageHandler) -> None:
        self._handler = handler

    def publish(self, topic: str, payload: bytes, qos: int = 1, retain: bool = False) -> None:
        self._client.publish(topic, payload, qos=qos, retain=retain)

    def connect(self) -> None:
        log.info("connecting to broker %s:%d", self._host, self._port)
        self._client.connect(self._host, self._port, keepalive=30)
        self._client.loop_start()

    def close(self) -> None:
        self._client.loop_stop()
        self._client.disconnect()

    def device_info(self) -> dict:
        return {"name": f"{self._host}:{self._port}", "address": None}

    def _on_connect(self, client, userdata, flags, reason_code, properties=None):
        log.info("broker connect rc=%s", reason_code)
        for topic in (
            topics.hello(self._device_id),
            topics.status(self._device_id),
            topics.acks(self._device_id),
        ):
            client.subscribe(topic, qos=1)
        self._set_state("connected")

    def _on_disconnect(self, client, userdata, flags, reason_code, properties=None):
        log.warning("broker disconnect rc=%s", reason_code)
        self._set_state("reconnecting")

    def _on_message(self, client, userdata, msg):
        if self._handler is None:
            return
        try:
            self._handler(msg.topic, msg.payload.decode("utf-8", "replace"))
        except Exception:  # noqa: BLE001
            log.exception("inbound handler failed for %s", msg.topic)
