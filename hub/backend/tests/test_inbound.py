from src.network import InboundRegistry, InboundRouter
from src.wire.protocol import TimeSet, envelope


def make_router():
    registry = InboundRegistry()
    router = InboundRouter(registry)
    return registry, router


def test_router_dispatches_typed_envelope():
    registry, router = make_router()
    seen = []
    router.on("time.set", lambda topic, env: seen.append((topic, env.body)))

    payload = envelope("time.set", TimeSet(epoch=1700000000), src="dev1")
    router.handle("tama/dev1/evt/time", payload)

    assert len(seen) == 1
    topic, body = seen[0]
    assert topic == "tama/dev1/evt/time"
    assert body["epoch"] == 1700000000
    assert "dev1" in registry.snapshot()


def test_router_ignores_unknown_types():
    _, router = make_router()
    seen = []
    router.on("time.set", lambda topic, env: seen.append(topic))

    payload = envelope("device.hello", TimeSet(epoch=1), src="dev1")
    router.handle("tama/dev1/evt/hello", payload)

    assert seen == []


def test_router_survives_garbage_payload():
    registry, router = make_router()
    router.handle("tama/dev1/evt/ack", "not json at all")

    info = registry.snapshot()["dev1"]
    assert info["last_body"] == {}
