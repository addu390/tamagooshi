from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass

from .base import Source
from .spec import SourceConfigBase


@dataclass(frozen=True)
class SourceProvider:
    type: str
    config_model: type[SourceConfigBase]
    factory: Callable[[SourceConfigBase], Source]
    label: str = ""
    description: str = ""


_PROVIDERS: dict[str, SourceProvider] = {}


def register(provider: SourceProvider) -> None:
    _PROVIDERS[provider.type] = provider


def provider(type_: str) -> SourceProvider:
    try:
        return _PROVIDERS[type_]
    except KeyError:
        known = ", ".join(sorted(_PROVIDERS)) or "none"
        raise ValueError(f"unknown source type: {type_!r} (known: {known})")


def providers() -> list[SourceProvider]:
    return [_PROVIDERS[type_] for type_ in sorted(_PROVIDERS)]


def parse_sources(raw: list[dict]) -> list[SourceConfigBase]:
    return [provider(item.get("type")).config_model.model_validate(item) for item in raw]


def build_source(cfg: SourceConfigBase) -> Source:
    return provider(cfg.type).factory(cfg)
