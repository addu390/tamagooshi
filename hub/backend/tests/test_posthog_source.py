import asyncio

import httpx
import pytest
from pydantic import ValidationError

from src.services.sources.posthog import PosthogMetric, PosthogSource, PosthogSourceConfig


@pytest.mark.parametrize("kw", [{}, {"query": "SELECT 1", "insight": "abc"}])
def test_metric_requires_exactly_one_source(kw):
    with pytest.raises(ValidationError):
        PosthogMetric(key="k", label="L", **kw)


def _cfg(metric: PosthogMetric, **over) -> PosthogSourceConfig:
    return PosthogSourceConfig(interval_secs=0.0, project_id="123", metrics=[metric], **over)


def _poll(src: PosthogSource, handler):
    async def run():
        async with httpx.AsyncClient(transport=httpx.MockTransport(handler)) as client:
            return await src.poll(client)

    return asyncio.run(run())


def test_hogql_reads_first_result_cell():
    metric = PosthogMetric(key="signups", label="SIGNUPS", query="SELECT count() FROM events")

    def handler(request: httpx.Request) -> httpx.Response:
        assert request.url.path == "/api/projects/123/query/"
        assert request.headers["Authorization"] == "Bearer key"
        assert request.method == "POST"
        return httpx.Response(200, json={"results": [[3200]]})

    updates = _poll(PosthogSource(_cfg(metric), api_key="key"), handler)

    assert len(updates) == 1
    assert updates[0].key == "signups"
    assert updates[0].value == "3.2k"
    assert updates[0].raw == 3200.0


def test_insight_reads_aggregated_value():
    metric = PosthogMetric(key="active", label="ACTIVE", insight="mmtixzFF")

    def handler(request: httpx.Request) -> httpx.Response:
        assert request.url.path == "/api/projects/123/insights/"
        assert request.url.params["short_id"] == "mmtixzFF"
        assert request.url.params["refresh"] == "blocking"
        assert request.method == "GET"
        return httpx.Response(200, json={"results": [{"result": [{"aggregated_value": 1234}]}]})

    updates = _poll(PosthogSource(_cfg(metric), api_key="key"), handler)

    assert updates[0].key == "active"
    assert updates[0].raw == 1234.0


def test_insight_falls_back_to_last_data_point():
    metric = PosthogMetric(key="dau", label="DAU", insight="abc123")

    def handler(_request: httpx.Request) -> httpx.Response:
        return httpx.Response(200, json={"results": [{"result": [{"data": [3, 7, 11]}]}]})

    updates = _poll(PosthogSource(_cfg(metric), api_key="key"), handler)

    assert updates[0].raw == 11.0


def test_host_trailing_slash_is_normalized():
    metric = PosthogMetric(key="s", label="S", query="SELECT 1")
    seen = {}

    def handler(request: httpx.Request) -> httpx.Response:
        seen["url"] = str(request.url)
        return httpx.Response(200, json={"results": [[1]]})

    _poll(PosthogSource(_cfg(metric, host="https://eu.posthog.com/"), api_key="key"), handler)

    assert seen["url"] == "https://eu.posthog.com/api/projects/123/query/"
