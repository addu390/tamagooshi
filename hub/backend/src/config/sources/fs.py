from __future__ import annotations

import os
import shutil

from ..codec import YAML, Codec
from ..source import BrandNotFound


# Order shared with firmware/tools/gen/manifest.py
def manifest_candidates(root: str, brand_id: str, extension: str = ".yaml") -> list[str]:
    return [os.path.join(root, brand_id, "config" + extension),
            os.path.join(root, brand_id + extension)]


class FileTreeSource:
    def __init__(self, root: str, codec: Codec = YAML):
        self._root = root
        self._codec = codec
        self._ext = codec.extensions[0]

    def ids(self) -> list[str]:
        if not os.path.isdir(self._root):
            return []

        found: dict[str, None] = {}
        for entry in sorted(os.listdir(self._root)):
            path = os.path.join(self._root, entry)
            if os.path.isdir(path) and os.path.isfile(os.path.join(path, "config" + self._ext)):
                found.setdefault(entry)
            elif entry.endswith(self._codec.extensions):
                found.setdefault(entry.rsplit(".", 1)[0])
        return list(found)

    def manifest(self, brand_id: str) -> dict:
        return self._read(self._resolve(brand_id))

    def scenes(self, brand_id: str) -> dict:
        path = os.path.join(os.path.dirname(self._resolve(brand_id)), "scenes" + self._ext)
        return self._read(path) if os.path.isfile(path) else {}

    def _resolve(self, brand_id: str) -> str:
        for candidate in manifest_candidates(self._root, brand_id, self._ext):
            if os.path.isfile(candidate):
                return candidate
        raise BrandNotFound(brand_id)

    def _read(self, path: str) -> dict:
        with open(path, "r", encoding="utf-8") as fh:
            return self._codec.load(fh.read())


class FileTreeStore(FileTreeSource):
    def write(self, brand_id: str, manifest: dict) -> None:
        folder, flat = manifest_candidates(self._root, brand_id, self._ext)
        path = folder if os.path.isfile(folder) else flat
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(self._codec.dump(manifest))

    def delete(self, brand_id: str) -> None:
        folder, flat = manifest_candidates(self._root, brand_id, self._ext)
        if os.path.isfile(folder):
            shutil.rmtree(os.path.dirname(folder))
        elif os.path.isfile(flat):
            os.remove(flat)
        else:
            raise BrandNotFound(brand_id)
