from __future__ import annotations

from typing import List, Optional

from pydantic import BaseModel, Field, SerializeAsAny

from ..model import AlertRule, Mood, MoodRule
from ..services.sources import SourceConfigBase


class BrokerConfig(BaseModel):
    host: str = "localhost"
    port: int = 1883


class AgentConfig(BaseModel):
    default: str = "cursor"
    enabled: List[str] = Field(default_factory=list)


class BrandConfig(BaseModel):
    name: str = "TAMAGOOSHI"
    tagline: Optional[str] = None
    logo_id: Optional[str] = None
    theme: Optional[str] = None
    mascot: Optional[str] = None
    carousel_secs: Optional[int] = None
    tz_offset: int = 0


class HubConfig(BaseModel):
    broker: BrokerConfig = Field(default_factory=BrokerConfig)
    device_id: str = "sim"
    brand_id: str = "gooshi"
    brand: BrandConfig = Field(default_factory=BrandConfig)
    agent: AgentConfig = Field(default_factory=AgentConfig)
    default_mood: Mood = "happy"
    sources: SerializeAsAny[List[SourceConfigBase]] = Field(default_factory=list)
    moods: List[MoodRule] = Field(default_factory=list)
    alerts: List[AlertRule] = Field(default_factory=list)
