"""Selectable agent backends: the single source of truth for agent ids and labels.

Data-only on purpose: the hub registry binds factories to these ids, and
firmware/tools/generator/catalog.py loads this file by path to publish the
same list to the web portal.
"""

AGENTS = {
    "cursor": "Cursor agent via cursor-sdk",
    "claude": "Claude agent via claude-agent-sdk",
}
