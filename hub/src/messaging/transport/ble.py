from __future__ import annotations

import asyncio
import logging
import struct
import threading
from typing import Optional

from bleak import BleakClient, BleakScanner

from .base import MessageHandler, Transport

log = logging.getLogger("tamagooshi.transport.ble")

SERVICE_UUID = "9e7b0001-8c9a-4f2b-8b7a-1e2d3c4b5a6f"
INBOUND_UUID = "9e7b0002-8c9a-4f2b-8b7a-1e2d3c4b5a6f"
OUTBOUND_UUID = "9e7b0003-8c9a-4f2b-8b7a-1e2d3c4b5a6f"
INFO_UUID = "9e7b0004-8c9a-4f2b-8b7a-1e2d3c4b5a6f"


def _frame(topic: str, payload: bytes) -> bytes:
    t = topic.encode("utf-8")
    msg = struct.pack(">H", len(t)) + t + payload
    return struct.pack(">I", len(msg)) + msg


class BleTransport(Transport):
    QUEUE_LIMIT = 256

    def __init__(self, name: Optional[str] = None, address: Optional[str] = None,
                 scan_timeout: float = 12.0):
        self._name = name
        self._address = address
        self._scan_timeout = scan_timeout
        self._handler: Optional[MessageHandler] = None
        self._loop = asyncio.new_event_loop()
        self._thread = threading.Thread(target=self._loop.run_forever, daemon=True)
        self._client: Optional[BleakClient] = None
        self._queue: asyncio.Queue = asyncio.Queue()
        self._rx = bytearray()
        self._closing = False

    def on_message(self, handler: MessageHandler) -> None:
        self._handler = handler

    def publish(self, topic: str, payload: bytes, qos: int = 1, retain: bool = False) -> None:
        self._loop.call_soon_threadsafe(self._enqueue, _frame(topic, payload))

    def connect(self) -> None:
        self._thread.start()
        asyncio.run_coroutine_threadsafe(self._connect(), self._loop).result(
            timeout=self._scan_timeout + 15
        )

    def close(self) -> None:
        self._closing = True
        if self._client is not None:
            try:
                asyncio.run_coroutine_threadsafe(self._client.disconnect(), self._loop).result(
                    timeout=10
                )
            except Exception:  # noqa: BLE001
                log.exception("ble disconnect failed")
        self._loop.call_soon_threadsafe(self._loop.stop)

    def _enqueue(self, data: bytes) -> None:
        while self._queue.qsize() >= self.QUEUE_LIMIT:
            self._queue.get_nowait()
            log.warning("outbound queue full; dropping oldest frame")
        self._queue.put_nowait(data)

    async def _find(self) -> str:
        if self._address:
            return self._address
        log.info("scanning for %s", self._name or "any tamagooshi device")
        devices = await BleakScanner.discover(timeout=self._scan_timeout, service_uuids=[SERVICE_UUID])
        for d in devices:
            if not self._name or (d.name and self._name.lower() in d.name.lower()):
                log.info("found %s (%s)", d.name, d.address)
                return d.address
        raise RuntimeError(f"no tamagooshi device matching '{self._name}' found")

    async def _connect(self) -> None:
        self._address = await self._find()
        await self._attach()
        self._loop.create_task(self._drain())

    async def _attach(self) -> None:
        client = BleakClient(self._address, disconnected_callback=self._on_disconnect)
        await client.connect()
        await client.start_notify(OUTBOUND_UUID, self._on_notify)
        self._client = client
        log.info("connected to %s", self._address)

    def _on_disconnect(self, _client) -> None:
        self._client = None
        if self._closing:
            return
        log.warning("ble link lost; reconnecting")
        self._loop.call_soon_threadsafe(lambda: self._loop.create_task(self._reconnect()))

    async def _reconnect(self) -> None:
        backoff = 1.0
        while not self._closing:
            try:
                await self._attach()
                return
            except Exception:  # noqa: BLE001
                log.warning("reconnect failed; retrying in %.0fs", backoff)
                await asyncio.sleep(backoff)
                backoff = min(backoff * 2, 30.0)

    async def _drain(self) -> None:
        while True:
            data = await self._queue.get()
            while not self._closing:
                client = self._client
                if client is None or not client.is_connected:
                    await asyncio.sleep(0.5)
                    continue
                try:
                    await self._write(data)
                    break
                except Exception:  # noqa: BLE001
                    log.exception("ble write failed; retrying")
                    await asyncio.sleep(1.0)

    async def _write(self, data: bytes) -> None:
        assert self._client is not None
        mtu = getattr(self._client, "mtu_size", 23) or 23
        chunk = max(20, mtu - 3)
        for off in range(0, len(data), chunk):
            await self._client.write_gatt_char(INBOUND_UUID, data[off:off + chunk], response=True)

    def _on_notify(self, _char, data: bytearray) -> None:
        self._rx.extend(data)
        while len(self._rx) >= 4:
            total = struct.unpack(">I", self._rx[0:4])[0]
            if len(self._rx) < 4 + total:
                break
            frame = bytes(self._rx[4:4 + total])
            del self._rx[0:4 + total]
            if total < 2:
                continue
            topic_len = struct.unpack(">H", frame[0:2])[0]
            if 2 + topic_len > total:
                continue
            topic = frame[2:2 + topic_len].decode("utf-8", "replace")
            payload = frame[2 + topic_len:].decode("utf-8", "replace")
            if self._handler is None:
                continue
            try:
                self._handler(topic, payload)
            except Exception:  # noqa: BLE001
                log.exception("inbound handler failed for %s", topic)
