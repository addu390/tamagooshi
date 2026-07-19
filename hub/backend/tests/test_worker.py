import asyncio

from src.config import HubConfig
from src.services.sources import parse_sources
from src.services.worker import Worker


class FakePipeline:
    def __init__(self):
        self.updates = []
        self.announced = False

    def announce(self):
        self.announced = True

    async def emit(self, update):
        self.updates.append(update)


DEMO = {"type": "demo", "interval_secs": 0.01,
        "metrics": [{"key": "m", "label": "M", "value_start": 5.0}]}
BROKEN = {"type": "posthog", "project_id": "1", "api_key_env": "TAMA_TEST_MISSING_KEY",
          "metrics": [{"key": "x", "label": "X", "query": "SELECT 1"}]}


def make_worker(*sources):
    config = HubConfig()
    config.sources = parse_sources(list(sources))
    pipeline = FakePipeline()
    return Worker(config, pipeline), pipeline


def test_missing_secret_does_not_crash_startup(monkeypatch):
    monkeypatch.delenv("TAMA_TEST_MISSING_KEY", raising=False)

    async def run():
        worker, pipeline = make_worker(BROKEN, DEMO)
        await worker.start()
        broken, demo = worker.snapshot()
        assert pipeline.announced
        assert not broken.running
        assert "TAMA_TEST_MISSING_KEY" in broken.error
        assert demo.running
        await worker.stop()

    asyncio.run(run())


def test_disabled_source_not_started_and_toggle():
    async def run():
        worker, _ = make_worker({**DEMO, "enabled": False})
        await worker.start()
        runtime = worker.snapshot()[0]
        assert not runtime.running

        runtime = worker.set_enabled(0, True)
        assert runtime.running and runtime.config.enabled

        runtime = worker.set_enabled(0, False)
        assert not runtime.running and not runtime.config.enabled
        await worker.stop()

    asyncio.run(run())


def test_refresh_polls_once_even_when_disabled():
    async def run():
        worker, pipeline = make_worker({**DEMO, "enabled": False})
        await worker.start()
        assert pipeline.updates == []

        await worker.refresh(0)
        assert [u.key for u in pipeline.updates] == ["m"]
        assert worker.snapshot()[0].last_emit is not None
        await worker.stop()

    asyncio.run(run())
