from __future__ import annotations

import logging
import os
from typing import Optional

from ...config.models import AgentConfig
from ...network.transport.base import LineChannel
from .bridge import VoiceBridge
from .agents.registry import create_agent, known_agents
from .transcriber import WhisperTranscriber

log = logging.getLogger("tamagooshi.buddy")

__all__ = ["VoiceBridge", "create_bridge"]


def _lineup(cfg: AgentConfig) -> Optional[tuple[str, list[str]]]:
    default = os.getenv("TAMA_AGENT", "").lower() or cfg.default
    if default in ("", "off", "none"):
        return None
    known = known_agents()
    if default not in known:
        log.warning("unknown agent '%s'; voice bridge disabled", default)
        return None
    enabled = [a for a in (cfg.enabled or [default]) if a in known]
    if default not in enabled:
        log.warning("agent '%s' is not enabled for this brand (enabled: %s); voice bridge disabled",
                    default, ", ".join(enabled))
        return None
    return default, enabled


def create_bridge(transport: object, cfg: AgentConfig) -> Optional[VoiceBridge]:
    lineup = _lineup(cfg)
    if lineup is None:
        return None
    if not isinstance(transport, LineChannel):
        log.info("transport has no agent line channel; voice bridge disabled")
        return None
    default, enabled = lineup
    transcriber = WhisperTranscriber(os.getenv("TAMA_WHISPER_MODEL", "base"))
    return VoiceBridge(transport, transcriber, create_agent, enabled, default)
