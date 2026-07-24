from __future__ import annotations

from pydantic import BaseModel, Field, SerializeAsAny

from ..model import AlertRule, Mood, MoodRule
from ..services.sources import SourceConfigBase


class BrokerConfig(BaseModel):
    host: str = "localhost"
    port: int = 1883


class AgentConfig(BaseModel):
    default: str = "cursor"
    enabled: list[str] = Field(default_factory=list)


class BrandConfig(BaseModel):
    name: str = "TAMAGOOSHI"
    tagline: str | None = None
    logo_id: str | None = None
    theme: str | None = None
    mascot: str | None = None
    carousel_secs: int | None = None
    tz_offset: int = 0


class HubConfig(BaseModel):
    broker: BrokerConfig = Field(default_factory=BrokerConfig)
    device_id: str = "sim"
    brand_id: str = "gooshi"
    brand: BrandConfig = Field(default_factory=BrandConfig)
    agent: AgentConfig = Field(default_factory=AgentConfig)
    default_mood: Mood = "happy"
    sources: SerializeAsAny[list[SourceConfigBase]] = Field(default_factory=list)
    moods: list[MoodRule] = Field(default_factory=list)
    alerts: list[AlertRule] = Field(default_factory=list)
