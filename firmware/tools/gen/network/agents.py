from gen.network.transports import LINK_MACRO
from gen.platform.boards import BOARDS

AGENTS = {
    "muse": {"macro": "TAMA_AGENT_MUSE", "links": ("ble", "wifi"), "caps": ("psram",)},
}


def parse_agent(value):
    if value is None:
        return None
    if value not in AGENTS:
        raise SystemExit(f"unknown agent: {value}")
    return value


def supports(agent, board):
    if agent is None or board is None:
        return agent is not None
    caps = BOARDS[board]["caps"]
    return all(caps.get(cap) for cap in AGENTS[agent]["caps"])


def agent_macros(agent, board):
    if not supports(agent, board):
        return []
    spec = AGENTS[agent]
    return sorted({spec["macro"], *(LINK_MACRO[link] for link in spec["links"])})
