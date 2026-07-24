import sys
import types

import pytest
from src.features.buddy.agents.base import AgentRunError, AgentStartupError


class TextBlock:
    def __init__(self, text):
        self.text = text


class AssistantMessage:
    def __init__(self, blocks):
        self.content = blocks


class CLINotFoundError(Exception):
    pass


class FakeClient:
    def __init__(self, responses=None, connect_error=None, receive_error=None):
        self.responses = responses or []
        self.connect_error = connect_error
        self.receive_error = receive_error
        self.queries = []
        self.connected = False
        self.disconnected = False

    async def connect(self):
        if self.connect_error is not None:
            raise self.connect_error
        self.connected = True

    async def disconnect(self):
        self.disconnected = True

    async def query(self, text):
        self.queries.append(text)

    async def receive_response(self):
        if self.receive_error is not None:
            raise self.receive_error
        for message in self.responses:
            yield message


@pytest.fixture
def sdk(monkeypatch):
    module = types.ModuleType("claude_agent_sdk")
    module.TextBlock = TextBlock
    module.AssistantMessage = AssistantMessage
    module.CLINotFoundError = CLINotFoundError
    module.ClaudeAgentOptions = lambda **kw: types.SimpleNamespace(**kw)
    module.ClaudeSDKClient = None
    monkeypatch.setitem(sys.modules, "claude_agent_sdk", module)
    return module


def make_backend():
    from src.features.buddy.agents.claude import ClaudeBackend

    return ClaudeBackend(cwd="/tmp/repo")


def test_send_streams_text_blocks(sdk):
    client = FakeClient(responses=[
        AssistantMessage([TextBlock("hi ")]),
        AssistantMessage([TextBlock("there")]),
    ])
    sdk.ClaudeSDKClient = lambda options: client

    backend = make_backend()
    deltas = []
    try:
        reply = backend.send("what changed?", deltas.append)
    finally:
        backend.close()

    assert reply == "hi there"
    assert deltas == ["hi ", "there"]
    assert client.queries == ["what changed?"]


def test_client_is_reused_across_turns(sdk):
    created = []

    def factory(options):
        client = FakeClient(responses=[AssistantMessage([TextBlock("ok")])])
        created.append(client)
        return client

    sdk.ClaudeSDKClient = factory
    backend = make_backend()
    try:
        backend.send("one", lambda _c: None)
        backend.send("two", lambda _c: None)
    finally:
        backend.close()

    assert len(created) == 1
    assert created[0].queries == ["one", "two"]


def test_connect_failure_maps_to_startup_error(sdk):
    sdk.ClaudeSDKClient = lambda options: FakeClient(connect_error=CLINotFoundError("no cli"))
    backend = make_backend()
    try:
        with pytest.raises(AgentStartupError):
            backend.send("hello", lambda _c: None)
    finally:
        backend.close()


def test_broken_stream_maps_to_run_error(sdk):
    client = FakeClient(receive_error=RuntimeError("stream died"))
    sdk.ClaudeSDKClient = lambda options: client
    backend = make_backend()
    try:
        with pytest.raises(AgentRunError):
            backend.send("hello", lambda _c: None)
    finally:
        backend.close()


def test_close_disconnects_client(sdk):
    client = FakeClient(responses=[AssistantMessage([TextBlock("ok")])])
    sdk.ClaudeSDKClient = lambda options: client
    backend = make_backend()
    backend.send("one", lambda _c: None)
    backend.close()

    assert client.disconnected
