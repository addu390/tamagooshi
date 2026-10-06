from __future__ import annotations

from pydantic import BaseModel, Field


class BrandConfig(BaseModel):
    name: str = "TAMAGOOSHI"
    tagline: str | None = None
    logo_id: str | None = None
    theme: str | None = None
    mascot: str | None = None
    carousel_secs: int | None = None
    tz_offset: int = 0


class HubConfig(BaseModel):
    device_id: str = "sim"
    brand_id: str = "gooshi"
    brand: BrandConfig = Field(default_factory=BrandConfig)
