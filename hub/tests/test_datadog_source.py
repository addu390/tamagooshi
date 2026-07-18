import asyncio

import httpx

from src.sources.datadog import DatadogMetric, DatadogSource, DatadogSourceConfig


def _cfg(**over) -> DatadogSourceConfig:
    return DatadogSourceConfig(
        interval_secs=0.0,
        metrics=[DatadogMetric(key="cpu", label="CPU", query="avg:system.cpu.user{*}", fmt="{v}%")],
        **over,
    )


def _poll(src: DatadogSource, handler):
    async def run():
        async with httpx.AsyncClient(transport=httpx.MockTransport(handler)) as client:
            return await src.poll(client)

    return asyncio.run(run())


def _poll_twice(src: DatadogSource, handler):
    async def run():
        async with httpx.AsyncClient(transport=httpx.MockTransport(handler)) as client:
            return await src.poll(client), await src.poll(client)

    return asyncio.run(run())


def test_poll_reads_latest_point_and_formats():
    def handler(request: httpx.Request) -> httpx.Response:
        assert request.url.path == "/api/v1/query"
        assert request.headers["DD-API-KEY"] == "key"
        assert request.headers["DD-APPLICATION-KEY"] == "app"
        return httpx.Response(200, json={"series": [{"pointlist": [[1, 41.0], [2, 63.4]]}]})

    updates = _poll(DatadogSource(_cfg(), api_key="key", app_key="app"), handler)

    assert len(updates) == 1
    assert updates[0].key == "cpu"
    assert updates[0].value == "63.4%"
    assert updates[0].raw == 63.4


def test_poll_computes_trend_across_calls():
    values = iter([10.0, 25.0])

    def handler(_request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, json={"series": [{"pointlist": [[1, next(values)]]}]})

    first, second = _poll_twice(DatadogSource(_cfg(), api_key="key", app_key="app"), handler)

    assert first[0].trend == "flat"
    assert second[0].trend == "up"


def test_poll_skips_metric_on_http_error():
    def handler(_request: httpx.Request) -> httpx.Response:
        return httpx.Response(500)

    updates = _poll(DatadogSource(_cfg(), api_key="key", app_key="app"), handler)

    assert updates == []
