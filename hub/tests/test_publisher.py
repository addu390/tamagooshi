import json

from src.config import BrandConfig
from src.model import AlertRule, Condition
from src.network import Publisher
from src.model import MetricUpdate
from src.wire import topics


class _FakeTransport:
    def __init__(self):
        self.published = []

    def publish(self, topic, payload, qos=1, retain=False):
        self.published.append((topic, payload, qos, retain))


def _publisher():
    fake = _FakeTransport()
    return Publisher(fake, "sim"), fake


def _last(fake):
    topic, payload, qos, retain = fake.published[-1]
    return topic, json.loads(payload), qos, retain


def test_state_messages_are_retained():
    pub, fake = _publisher()
    pub.publish_branding(BrandConfig(name="ACME"))
    topic, env, qos, retain = _last(fake)
    assert topic == topics.branding("sim")
    assert env["type"] == "branding.set"
    assert qos == 1 and retain is True


def test_metric_is_retained_per_key():
    pub, fake = _publisher()
    pub.publish_metric(MetricUpdate(key="mrr", label="MRR", value="$1k", kind="star"))
    topic, env, _, retain = _last(fake)
    assert topic == topics.metric("sim", "mrr")
    assert retain is True


def test_pages_are_not_retained():
    pub, fake = _publisher()
    rule = AlertRule(id="pg1", when=Condition(metric="uptime", op="lt", value=95.0), title="down")
    pub.publish_page(rule)
    topic, env, _, retain = _last(fake)
    assert topic == topics.pages("sim")
    assert env["type"] == "page.raise"
    assert retain is False

    pub.clear_page("pg1")
    _, env, _, retain = _last(fake)
    assert env["type"] == "page.clear"
    assert retain is False


def test_config_publishes_theme_and_mascot():
    pub, fake = _publisher()
    pub.publish_config(BrandConfig(name="ACME", theme="slate", mascot="cat"))
    topic, env, _, retain = _last(fake)
    assert topic == topics.config("sim")
    assert env["body"] == {"theme": "slate", "character_id": "cat"}
    assert retain is True
