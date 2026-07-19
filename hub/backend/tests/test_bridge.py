import asyncio
import base64
import json

from src.wire.codec import adpcm
from src.features.buddy.agents.base import AgentBackend, AgentStartupError
from src.features.buddy.bridge import VoiceBridge
from src.features.buddy.transcriber import Transcriber


class FakeChannel:
    def __init__(self):
        self.sent = []
        self.handler = None

    def on_line(self, handler):
        self.handler = handler

    def send_line(self, line):
        self.sent.append(json.loads(line))


class FakeTranscriber(Transcriber):
    def __init__(self, text="list open PRs"):
        self.text = text
        self.received = None

    def transcribe(self, pcm, sample_rate):
        self.received = (pcm, sample_rate)
        return self.text


class FakeBackend(AgentBackend):
    def __init__(self, name="cursor"):
        self.name = name
        self.prompts = []
        self.closed = False

    def send(self, text, on_delta):
        self.prompts.append(text)
        on_delta("hello ")
        on_delta("world")
        return "hello world"

    def close(self):
        self.closed = True


class FailingBackend(AgentBackend):
    def send(self, text, on_delta):
        raise AgentStartupError("CURSOR_API_KEY is not set")

    def close(self):
        pass


def make_bridge(transcriber=None, backend=None, enabled=("cursor", "claude")):
    channel = FakeChannel()
    backend = backend or FakeBackend()
    created = []

    def factory(agent):
        created.append(agent)
        return backend

    bridge = VoiceBridge(channel, transcriber or FakeTranscriber(), factory,
                         enabled=list(enabled), default="cursor")
    bridge._created = created
    return bridge, channel, backend


async def upload_voice(bridge, payload=b"\x12\x34" * 40, agent=None):
    data = base64.b64encode(payload).decode("ascii")
    await bridge._handle(json.dumps({"cmd": "voice", "seq": 0, "data": data}))
    end = {"cmd": "voice_end", "ms": 1200}
    if agent is not None:
        end["agent"] = agent
    await bridge._handle(json.dumps(end))
    return payload


def events(channel, kind):
    return [m for m in channel.sent if m.get("evt") == kind]


def test_voice_end_sends_transcript():
    async def run():
        bridge, channel, _ = make_bridge()
        payload = await upload_voice(bridge)

        transcripts = events(channel, "transcript")
        assert len(transcripts) == 1
        assert transcripts[0]["text"] == "list open PRs"
        assert transcripts[0]["id"] == "v1"
        assert transcripts[0]["agent"] == "cursor"
        assert bridge._transcriber.received[0] == adpcm.decode(payload)

    asyncio.run(run())


def test_approved_transcript_reaches_backend_and_replies():
    async def run():
        bridge, channel, backend = make_bridge()
        await upload_voice(bridge)
        voice_id = events(channel, "transcript")[0]["id"]

        await bridge._handle(json.dumps({"cmd": "permission", "id": voice_id,
                                         "decision": "once"}))

        assert backend.prompts == ["list open PRs"]
        replies = events(channel, "reply")
        assert "".join(r["text"] for r in replies) == "hello world"
        assert replies[-1]["done"] is True

        snapshots = [m for m in channel.sent if "running" in m and "evt" not in m]
        assert snapshots[0]["running"] == 1
        assert snapshots[-1]["running"] == 0

    asyncio.run(run())


def test_denied_transcript_never_reaches_backend():
    async def run():
        bridge, channel, backend = make_bridge()
        await upload_voice(bridge)
        voice_id = events(channel, "transcript")[0]["id"]

        await bridge._handle(json.dumps({"cmd": "permission", "id": voice_id,
                                         "decision": "deny"}))

        assert backend.prompts == []
        assert events(channel, "reply") == []

    asyncio.run(run())


def test_empty_transcript_reports_instead_of_prompting():
    async def run():
        bridge, channel, _ = make_bridge(transcriber=FakeTranscriber(text=""))
        await upload_voice(bridge)

        assert events(channel, "transcript") == []
        replies = events(channel, "reply")
        assert replies and replies[0]["done"] is True

    asyncio.run(run())


def test_broken_chunk_stream_reports_error():
    async def run():
        bridge, channel, _ = make_bridge()
        await bridge._handle(json.dumps({"cmd": "voice", "seq": 0, "data": "aGk="}))
        await bridge._handle(json.dumps({"cmd": "voice", "seq": 4, "data": "aGk="}))
        await bridge._handle(json.dumps({"cmd": "voice_end", "ms": 500}))

        assert events(channel, "transcript") == []
        assert events(channel, "reply")[0]["done"] is True

    asyncio.run(run())


def test_startup_error_is_reported_to_device():
    async def run():
        bridge, channel, _ = make_bridge(backend=FailingBackend())
        await upload_voice(bridge)
        voice_id = events(channel, "transcript")[0]["id"]

        await bridge._handle(json.dumps({"cmd": "permission", "id": voice_id,
                                         "decision": "once"}))

        replies = events(channel, "reply")
        assert replies and replies[-1]["done"] is True
        assert "failed to start" in replies[-1]["text"]

    asyncio.run(run())


def test_unknown_permission_id_is_ignored():
    async def run():
        bridge, channel, backend = make_bridge()
        await bridge._handle(json.dumps({"cmd": "permission", "id": "req_abc",
                                         "decision": "once"}))
        assert backend.prompts == []

    asyncio.run(run())


def test_agents_request_returns_roster():
    async def run():
        bridge, channel, _ = make_bridge()
        await bridge._handle(json.dumps({"cmd": "agents"}))

        rosters = events(channel, "agents")
        assert rosters == [{"evt": "agents", "enabled": ["cursor", "claude"],
                            "default": "cursor"}]

    asyncio.run(run())


def test_device_chosen_agent_is_used_and_stamped():
    async def run():
        bridge, channel, _ = make_bridge()
        await upload_voice(bridge, agent="claude")

        assert events(channel, "transcript")[0]["agent"] == "claude"

        voice_id = events(channel, "transcript")[0]["id"]
        await bridge._handle(json.dumps({"cmd": "permission", "id": voice_id,
                                         "decision": "once"}))
        assert bridge._created == ["claude"]

    asyncio.run(run())


def test_unknown_agent_falls_back_to_default():
    async def run():
        bridge, channel, _ = make_bridge()
        await upload_voice(bridge, agent="gemini")

        assert events(channel, "transcript")[0]["agent"] == "cursor"

    asyncio.run(run())


def test_backend_is_created_once_per_agent():
    async def run():
        bridge, channel, _ = make_bridge()
        for _ in range(2):
            await upload_voice(bridge, agent="claude")
            voice_id = events(channel, "transcript")[-1]["id"]
            await bridge._handle(json.dumps({"cmd": "permission", "id": voice_id,
                                             "decision": "once"}))
        assert bridge._created == ["claude"]

    asyncio.run(run())


def test_stop_closes_created_backends():
    async def run():
        bridge, channel, backend = make_bridge()
        await upload_voice(bridge, agent="cursor")
        voice_id = events(channel, "transcript")[0]["id"]
        await bridge._handle(json.dumps({"cmd": "permission", "id": voice_id,
                                         "decision": "once"}))
        await bridge.stop()
        assert backend.closed

    asyncio.run(run())
