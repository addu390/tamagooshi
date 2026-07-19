"""Selectable agent backends: the single source of truth for agent metadata.

Data-only on purpose: the hub registry binds factories to these ids, the API
reports tooling availability from the cli and sdk fields, and
firmware/tools/gen/emit/catalog.py loads this file by path to publish the
same list to the web portal.
"""

CATALOG = {
    "cursor": {
        "label": "Cursor agent via cursor-sdk",
        "cli": "cursor-agent",
        "sdk": "cursor_sdk",
    },
    "claude": {
        "label": "Claude agent via claude-agent-sdk",
        "cli": "claude",
        "sdk": "claude_agent_sdk",
    },
}

AGENTS = {aid: entry["label"] for aid, entry in CATALOG.items()}
