from __future__ import annotations

from .source import BrandNotFound, BrandSource, BrandStore


class LayeredCatalog:
    """Brand storage where the user layer overrides builtin and receives all writes."""

    def __init__(self, user: BrandStore, builtin: BrandSource):
        self._user = user
        self._builtin = builtin

    def ids(self) -> list[str]:
        found: dict[str, None] = {}
        for _, layer in self._layers():
            for brand_id in layer.ids():
                found.setdefault(brand_id)
        return list(found)

    def manifest(self, brand_id: str) -> dict:
        return self._layer(brand_id).manifest(brand_id)

    def scenes(self, brand_id: str) -> dict:
        return self._layer(brand_id).scenes(brand_id)

    def origin(self, brand_id: str) -> str:
        for name, layer in self._layers():
            if brand_id in layer.ids():
                return name
        raise BrandNotFound(brand_id)

    def exists(self, brand_id: str) -> bool:
        return any(brand_id in layer.ids() for _, layer in self._layers())

    def in_user(self, brand_id: str) -> bool:
        return brand_id in self._user.ids()

    def in_builtin(self, brand_id: str) -> bool:
        return brand_id in self._builtin.ids()

    def write(self, brand_id: str, manifest: dict) -> None:
        self._user.write(brand_id, manifest)

    def delete_user(self, brand_id: str) -> None:
        self._user.delete(brand_id)

    def _layers(self) -> tuple[tuple[str, BrandSource], ...]:
        return (("user", self._user), ("builtin", self._builtin))

    def _layer(self, brand_id: str) -> BrandSource:
        for _, layer in self._layers():
            if brand_id in layer.ids():
                return layer
        raise BrandNotFound(brand_id)
