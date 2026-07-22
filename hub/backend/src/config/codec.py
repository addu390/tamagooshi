from __future__ import annotations

from typing import Protocol

import yaml


class Codec(Protocol):
    extensions: tuple[str, ...]
    media_type: str

    def load(self, text: str | bytes) -> dict: ...

    def dump(self, data: dict) -> str: ...


class YamlCodec:
    extensions = (".yaml", ".yml")
    media_type = "application/x-yaml"

    def load(self, text: str | bytes) -> dict:
        return yaml.safe_load(text) or {}

    def dump(self, data: dict) -> str:
        return yaml.safe_dump(data, sort_keys=False, allow_unicode=True)


YAML = YamlCodec()
