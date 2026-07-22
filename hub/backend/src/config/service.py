from __future__ import annotations

import re

from .catalog import LayeredCatalog
from .loader import hub_config_from_manifest

IDENTITY_FIELDS = ("name", "tagline", "website", "mascot")
BRAND_ID = re.compile(r"[a-z0-9][a-z0-9-]*")
TEMPLATE_BRAND = "template"


class BrandService:
    def __init__(self, catalog: LayeredCatalog):
        self._catalog = catalog

    def read_manifest(self, brand_id: str) -> dict:
        return self._catalog.manifest(brand_id)

    def mutate_manifest(self, brand_id: str, mutate) -> dict:
        data = self.read_manifest(brand_id)
        mutate(data)

        hub_config_from_manifest(data)
        self._catalog.write(brand_id, data)
        return data

    def list_brands(self) -> list[dict]:
        out = []
        for brand_id in self._catalog.ids():
            ident = self.read_manifest(brand_id).get("brand") or {}
            origin = self._catalog.origin(brand_id)
            out.append({
                "id": brand_id,
                "name": ident.get("name", brand_id),
                "tagline": ident.get("tagline"),
                "source": origin,
                "overrides_builtin": origin == "user" and self._catalog.in_builtin(brand_id),
            })
        return out

    def exists(self, brand_id: str) -> bool:
        return self._catalog.exists(brand_id)

    def has_user_copy(self, brand_id: str) -> bool:
        return self._catalog.in_user(brand_id)

    def builtin_exists(self, brand_id: str) -> bool:
        return self._catalog.in_builtin(brand_id)

    def import_manifest(self, data: dict) -> str:
        hub_config_from_manifest(data)

        brand_id = (data.get("brand") or {}).get("id")
        if not brand_id:
            raise ValueError("brand.id missing")

        self._catalog.write(brand_id, data)
        return brand_id

    def create_brand(self, brand_id: str, name: str, tagline: str = "") -> str:
        if not BRAND_ID.fullmatch(brand_id):
            raise ValueError("id must be lowercase letters, numbers and dashes")
        if not name:
            raise ValueError("name is required")
        if self._catalog.exists(brand_id):
            raise ValueError(f"brand '{brand_id}' already exists")

        data = self.read_manifest(TEMPLATE_BRAND)
        data["brand"] = {**(data.get("brand") or {}), "id": brand_id, "name": name,
                         "tagline": tagline}
        hub_config_from_manifest(data)

        self._catalog.write(brand_id, data)
        return brand_id

    def delete_user_brand(self, brand_id: str) -> None:
        self._catalog.delete_user(brand_id)

    def set_source_enabled(self, brand_id: str, index: int, enabled: bool) -> None:
        def mutate(sources: list) -> None:
            sources[index]["enabled"] = enabled

        self._mutate_sources(brand_id, mutate)

    def add_source(self, brand_id: str, source: dict) -> None:
        self._mutate_sources(brand_id, lambda sources: sources.append(source))

    def update_source(self, brand_id: str, index: int, source: dict) -> None:
        def mutate(sources: list) -> None:
            sources[index] = source

        self._mutate_sources(brand_id, mutate)

    def remove_source(self, brand_id: str, index: int) -> None:
        self._mutate_sources(brand_id, lambda sources: sources.pop(index))

    def update_identity(self, brand_id: str, identity: dict) -> dict:
        def apply(data: dict) -> None:
            brand = data.setdefault("brand", {})
            for field in IDENTITY_FIELDS:
                if field not in identity:
                    continue
                value = identity[field]
                if value in (None, ""):
                    brand.pop(field, None)
                else:
                    brand[field] = value

        return self.mutate_manifest(brand_id, apply)

    def update_device(self, brand_id: str, device: dict) -> dict:
        def apply(data: dict) -> None:
            data["device"] = device

        return self.mutate_manifest(brand_id, apply)

    def update_rules(self, brand_id: str, moods: list | None = None,
                     alerts: list | None = None) -> dict:
        def apply(data: dict) -> None:
            hub = data.setdefault("hub", {})
            if moods is not None:
                hub["moods"] = moods
            if alerts is not None:
                hub["alerts"] = alerts

        return self.mutate_manifest(brand_id, apply)

    def update_agent(self, brand_id: str, default: str, enabled: list) -> dict:
        def apply(data: dict) -> None:
            data.setdefault("hub", {})["agent"] = {"default": default, "enabled": enabled}

        return self.mutate_manifest(brand_id, apply)

    def _mutate_sources(self, brand_id: str, mutate) -> dict:
        def apply(data: dict) -> None:
            mutate(data.setdefault("hub", {}).setdefault("sources", []))

        return self.mutate_manifest(brand_id, apply)
