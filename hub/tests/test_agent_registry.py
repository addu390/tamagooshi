import pytest

from src.features.buddy import create_bridge
from src.features.buddy.agents.catalog import AGENTS
from src.features.buddy.agents.registry import _FACTORIES, create_agent, known_agents
from src.config.models import AgentConfig
from src.network.transport.base import LineChannel


class FakeChannel(LineChannel):
    def on_line(self, handler):
        self.handler = handler

    def send_line(self, line):
        pass


class FakeBackend:
    def send(self, text, on_delta):
        return ""

    def close(self):
        pass


@pytest.fixture
def stub_backend(monkeypatch):
    import src.features.buddy as buddy

    monkeypatch.setattr(buddy, "create_agent", lambda kind: FakeBackend())


def test_known_agents_cover_cursor_and_claude():
    assert known_agents() == ["claude", "cursor"]


def test_every_cataloged_agent_has_a_factory():
    assert set(_FACTORIES) == set(AGENTS)


def test_unknown_agent_raises_with_known_list():
    with pytest.raises(ValueError, match="claude, cursor"):
        create_agent("gemini")


def test_bridge_uses_brand_default(stub_backend, monkeypatch):
    monkeypatch.delenv("TAMA_AGENT", raising=False)
    cfg = AgentConfig(default="cursor", enabled=["cursor", "claude"])
    bridge = create_bridge(FakeChannel(), cfg)
    assert bridge is not None
    assert bridge._default == "cursor"
    assert bridge._enabled == ["cursor", "claude"]


def test_env_overrides_brand_default(stub_backend, monkeypatch):
    monkeypatch.setenv("TAMA_AGENT", "claude")
    cfg = AgentConfig(default="cursor", enabled=["cursor", "claude"])
    bridge = create_bridge(FakeChannel(), cfg)
    assert bridge is not None
    assert bridge._default == "claude"


def test_enabled_list_drops_unknown_agents(stub_backend, monkeypatch):
    monkeypatch.delenv("TAMA_AGENT", raising=False)
    cfg = AgentConfig(default="cursor", enabled=["cursor", "gemini"])
    bridge = create_bridge(FakeChannel(), cfg)
    assert bridge is not None
    assert bridge._enabled == ["cursor"]


def test_agent_off_disables_bridge(monkeypatch):
    monkeypatch.setenv("TAMA_AGENT", "off")
    assert create_bridge(FakeChannel(), AgentConfig()) is None


def test_agent_outside_enabled_list_is_rejected(monkeypatch):
    monkeypatch.setenv("TAMA_AGENT", "claude")
    cfg = AgentConfig(default="cursor", enabled=["cursor"])
    assert create_bridge(FakeChannel(), cfg) is None


def test_unknown_agent_disables_bridge(monkeypatch):
    monkeypatch.setenv("TAMA_AGENT", "gemini")
    assert create_bridge(FakeChannel(), AgentConfig()) is None


def test_channel_without_lines_disables_bridge(monkeypatch):
    monkeypatch.delenv("TAMA_AGENT", raising=False)
    assert create_bridge(object(), AgentConfig(default="cursor")) is None
