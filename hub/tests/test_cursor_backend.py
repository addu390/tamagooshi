import sys
import types

import pytest

from src.features.buddy.agents.base import AgentRunError, AgentStartupError


class FakeRun:
    def __init__(self, chunks, status="finished"):
        self._chunks = chunks
        self._status = status
        self.id = "run-1"
        self.waited = False

    def iter_text(self):
        yield from self._chunks

    def wait(self):
        self.waited = True
        return types.SimpleNamespace(status=self._status, id=self.id)


class FakeAgent:
    def __init__(self, run):
        self._run = run
        self.agent_id = "agent-1"
        self.sent = []
        self.closed = False

    def send(self, text):
        self.sent.append(text)
        return self._run

    def close(self):
        self.closed = True


class FakeCursorAgentError(Exception):
    pass


@pytest.fixture
def sdk(monkeypatch):
    module = types.ModuleType("cursor_sdk")
    module.CursorAgentError = FakeCursorAgentError
    module.LocalAgentOptions = lambda cwd: types.SimpleNamespace(cwd=cwd)
    module.Agent = types.SimpleNamespace(create=None)
    monkeypatch.setitem(sys.modules, "cursor_sdk", module)
    return module


def make_backend(**kwargs):
    from src.features.buddy.agents.cursor import CursorBackend

    return CursorBackend(cwd="/tmp/repo", api_key="cursor_test", **kwargs)


def test_send_streams_and_returns_reply(sdk):
    run = FakeRun(["hi ", "there"])
    agent = FakeAgent(run)
    sdk.Agent.create = lambda **kw: agent

    backend = make_backend()
    deltas = []
    reply = backend.send("what changed?", deltas.append)

    assert reply == "hi there"
    assert deltas == ["hi ", "there"]
    assert agent.sent == ["what changed?"]
    assert run.waited


def test_agent_is_reused_across_turns(sdk):
    created = []

    def create(**kw):
        agent = FakeAgent(FakeRun(["ok"]))
        created.append(agent)
        return agent

    sdk.Agent.create = create
    backend = make_backend()
    backend.send("one", lambda _c: None)
    backend.send("two", lambda _c: None)

    assert len(created) == 1
    assert created[0].sent == ["one", "two"]


def test_startup_failure_maps_to_startup_error(sdk):
    def create(**kw):
        raise FakeCursorAgentError("bad key")

    sdk.Agent.create = create
    backend = make_backend()
    with pytest.raises(AgentStartupError):
        backend.send("hello", lambda _c: None)


def test_missing_api_key_is_a_startup_error(sdk, monkeypatch):
    from src.features.buddy.agents.cursor import CursorBackend

    monkeypatch.delenv("CURSOR_API_KEY", raising=False)
    backend = CursorBackend(cwd="/tmp/repo")
    with pytest.raises(AgentStartupError):
        backend.send("hello", lambda _c: None)


def test_failed_run_maps_to_run_error(sdk):
    sdk.Agent.create = lambda **kw: FakeAgent(FakeRun(["boom"], status="error"))
    backend = make_backend()
    with pytest.raises(AgentRunError):
        backend.send("hello", lambda _c: None)


def test_close_disposes_agent(sdk):
    agent = FakeAgent(FakeRun(["ok"]))
    sdk.Agent.create = lambda **kw: agent
    backend = make_backend()
    backend.send("one", lambda _c: None)
    backend.close()

    assert agent.closed
