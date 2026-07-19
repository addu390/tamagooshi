import asyncio

import httpx

from src.services.sources import MetricSpec, PollingSource
from src.services.sources.demo import DemoSourceConfig
from src.services.sources.demo.source import DemoSource


class FlakySource(PollingSource):
    name = "flaky"

    async def fetch(self, client: httpx.AsyncClient, metric: MetricSpec) -> float:
        if metric.key == "bad":
            raise ValueError("query returned no rows")
        return 42.0


def make_flaky():
    return FlakySource(0.01, [MetricSpec(key="good", label="GOOD"),
                              MetricSpec(key="bad", label="BAD")])


def test_polling_source_tracks_per_metric_status():
    async def run():
        source = make_flaky()
        emitted = []

        async def emit(update):
            emitted.append(update.key)

        await source.poll_once(emit)
        status = source.metric_status()
        assert emitted == ["good"]
        assert status["good"]["error"] is None
        assert status["good"]["value"] == "42.0"
        assert "no rows" in status["bad"]["error"]
        assert status["bad"]["value"] is None

    asyncio.run(run())


def test_demo_source_reports_status():
    async def run():
        cfg = DemoSourceConfig.model_validate(
            {"type": "demo", "metrics": [{"key": "m", "label": "M", "value_start": 5.0}]}
        )
        source = DemoSource(cfg)

        async def emit(update):
            pass

        await source.poll_once(emit)
        status = source.metric_status()
        assert status["m"]["error"] is None
        assert status["m"]["ts"] is not None

    asyncio.run(run())
