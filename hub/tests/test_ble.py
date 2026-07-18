import struct

import pytest

pytest.importorskip("bleak")

from src.messaging.transport.ble import BleTransport, _frame


def make_transport():
    return BleTransport(address="AA:BB:CC:DD:EE:FF")


def test_frame_roundtrip_via_notify():
    t = make_transport()
    seen = []
    t.on_message(lambda topic, payload: seen.append((topic, payload)))

    t._on_notify(None, bytearray(_frame("tama/dev/metric", b'{"v":1}')))

    assert seen == [("tama/dev/metric", '{"v":1}')]


def test_notify_reassembles_chunks():
    t = make_transport()
    seen = []
    t.on_message(lambda topic, payload: seen.append((topic, payload)))

    data = _frame("t/a", b"hello") + _frame("t/b", b"world")
    for i in range(0, len(data), 3):
        t._on_notify(None, bytearray(data[i:i + 3]))

    assert seen == [("t/a", "hello"), ("t/b", "world")]


def test_notify_skips_bad_topic_length():
    t = make_transport()
    seen = []
    t.on_message(lambda topic, payload: seen.append(topic))

    msg = struct.pack(">H", 999) + b"xx"
    t._on_notify(None, bytearray(struct.pack(">I", len(msg)) + msg))

    assert seen == []


def test_enqueue_drops_oldest_when_full():
    t = make_transport()
    for i in range(BleTransport.QUEUE_LIMIT + 5):
        t._enqueue(bytes([i % 256]))

    assert t._queue.qsize() == BleTransport.QUEUE_LIMIT
    assert t._queue.get_nowait() == bytes([5])
