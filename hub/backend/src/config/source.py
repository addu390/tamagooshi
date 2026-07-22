from __future__ import annotations

from typing import Protocol


class BrandNotFound(LookupError):
    def __init__(self, brand_id: str):
        super().__init__(f"brand '{brand_id}' not found")
        self.brand_id = brand_id


class BrandSource(Protocol):
    def ids(self) -> list[str]: ...

    def manifest(self, brand_id: str) -> dict: ...

    def scenes(self, brand_id: str) -> dict: ...


class BrandStore(BrandSource, Protocol):
    def write(self, brand_id: str, manifest: dict) -> None: ...

    def delete(self, brand_id: str) -> None: ...
