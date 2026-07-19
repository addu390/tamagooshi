from __future__ import annotations

import json
import os
import sys


def data_dir() -> str:
    override = os.environ.get("TAMA_DATA_DIR")
    if override:
        return override
    if sys.platform == "darwin":
        return os.path.expanduser("~/Library/Application Support/Tamagooshi")
    return os.path.expanduser("~/.config/tamagooshi")


def user_brands_dir() -> str:
    return os.path.join(data_dir(), "brands")


def _settings_path() -> str:
    return os.path.join(data_dir(), "settings.json")


def load_settings() -> dict:
    try:
        with open(_settings_path(), "r", encoding="utf-8") as fh:
            return json.load(fh) or {}
    except (FileNotFoundError, json.JSONDecodeError):
        return {}


def save_settings(settings: dict) -> None:
    os.makedirs(data_dir(), exist_ok=True)
    with open(_settings_path(), "w", encoding="utf-8") as fh:
        json.dump(settings, fh, indent=2)


def set_active_brand(brand_id: str) -> None:
    settings = load_settings()
    settings["brand"] = brand_id
    save_settings(settings)


def load_connection() -> dict:
    return load_settings().get("connection") or {}


def save_connection(connection: dict) -> None:
    settings = load_settings()
    settings["connection"] = connection
    save_settings(settings)
