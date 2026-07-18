import random

from src.sources.demo import DemoMetric, DemoSource, DemoSourceConfig


def _source(drift: float = 900.0) -> DemoSource:
    cfg = DemoSourceConfig(
        interval_secs=0.0,
        metrics=[
            DemoMetric(key="mrr", label="MRR", value_start=48000, fmt="${v}", kind="star", drift=drift),
            DemoMetric(key="uptime", label="UPTIME", value_start=99.9, fmt="{v}%", drift=drift),
        ],
    )
    return DemoSource(cfg, rng=random.Random(42))


def test_tick_emits_one_update_per_metric():
    updates = _source().tick()
    assert [u.key for u in updates] == ["mrr", "uptime"]


def test_star_metric_kind_and_format():
    mrr = _source().tick()[0]
    assert mrr.kind == "star"
    assert mrr.value.startswith("$") and mrr.value.endswith("k")


def test_trend_flat_without_drift():
    updates = _source(drift=0.0).tick()
    assert all(u.trend == "flat" for u in updates)


def test_value_never_negative():
    src = _source(drift=1_000_000.0)
    for _ in range(50):
        for u in src.tick():
            assert not u.value.lstrip("$").startswith("-")
