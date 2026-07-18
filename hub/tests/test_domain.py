from src.domain import AlertRule, Condition, MoodRule
from src.domain import AlertEngine, MetricStore, MoodEngine
from src.domain import MetricUpdate


def _store(**vals) -> MetricStore:
    s = MetricStore()
    for k, v in vals.items():
        s.update(MetricUpdate(key=k, label=k, value=str(v), raw=v))
    return s


def _mood_rules():
    return [
        MoodRule(when=Condition(metric="uptime", op="lt", value=95.0), mood="panic", priority=20),
        MoodRule(when=Condition(metric="uptime", op="lt", value=99.0), mood="sick", priority=10),
        MoodRule(when=Condition(metric="mrr", op="gte", value=60000.0), mood="celebrate", priority=5),
    ]


def test_mood_default_when_no_match():
    eng = MoodEngine(_mood_rules(), "happy")
    assert eng.evaluate(_store(uptime=99.95, mrr=48000)) == "happy"


def test_mood_highest_priority_wins():
    eng = MoodEngine(_mood_rules(), "happy")
    assert eng.evaluate(_store(uptime=92.0)) == "panic"


def test_mood_celebrate():
    eng = MoodEngine(_mood_rules(), "happy")
    assert eng.evaluate(_store(uptime=99.99, mrr=61000)) == "celebrate"


def test_mood_missing_metric_is_ignored():
    eng = MoodEngine(_mood_rules(), "happy")
    assert eng.evaluate(_store()) == "happy"


def _alert():
    return AlertEngine(
        [AlertRule(id="uptime-critical", when=Condition(metric="uptime", op="lt", value=95.0),
                   severity="critical", title="Uptime dropping")]
    )


def test_alert_edge_triggered_raise_and_clear():
    eng = _alert()
    t1 = eng.evaluate(_store(uptime=99.0))
    assert not t1.raised and not t1.cleared

    t2 = eng.evaluate(_store(uptime=92.0))
    assert [r.id for r in t2.raised] == ["uptime-critical"]
    assert not t2.cleared

    t3 = eng.evaluate(_store(uptime=93.0))
    assert not t3.raised and not t3.cleared

    t4 = eng.evaluate(_store(uptime=99.0))
    assert t4.cleared == ["uptime-critical"]


def test_alert_ack_tracked_and_reset_on_clear():
    eng = _alert()
    eng.evaluate(_store(uptime=92.0))
    eng.acknowledge("uptime-critical")
    assert eng.snapshot()["acknowledged"] == ["uptime-critical"]

    eng.evaluate(_store(uptime=99.0))
    assert eng.snapshot()["acknowledged"] == []


def test_ack_ignored_when_not_active():
    eng = _alert()
    eng.acknowledge("uptime-critical")
    assert eng.snapshot()["acknowledged"] == []
