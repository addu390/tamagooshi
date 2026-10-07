import json

from src.config import BrandConfig
from src.network import Publisher
from src.wire import topics


class _FakeTransport:
    def __init__(self):
        self.published = []

    def publish(self, topic, payload):
        self.published.append((topic, payload))


def _publisher():
    fake = _FakeTransport()
    return Publisher(fake, "sim"), fake


def _last(fake):
    topic, payload = fake.published[-1]
    return topic, json.loads(payload)


def test_publish_branding():
    pub, fake = _publisher()
    pub.publish_branding(BrandConfig(name="ACME"))
    topic, env = _last(fake)
    assert topic == topics.branding("sim")
    assert env["type"] == "branding.set"


def test_config_publishes_theme_and_mascot():
    pub, fake = _publisher()
    pub.publish_config(BrandConfig(name="ACME", theme="slate", mascot="cat"))
    topic, env = _last(fake)
    assert topic == topics.config("sim")
    assert env["body"] == {"theme": "slate", "character_id": "cat"}


def test_publish_hid_mode():
    pub, fake = _publisher()
    pub.publish_hid_mode("desk")
    topic, env = _last(fake)
    assert topic == topics.config("sim")
    assert env["type"] == "config.set"
    assert env["body"] == {"hid_mode": "desk"}
