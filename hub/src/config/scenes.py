from __future__ import annotations

import os

import yaml

from ..model import AlertRule, MoodRule
from ..services.sources import parse_sources
from .models import HubConfig


def apply_scene(cfg: HubConfig, manifest_path: str, scene: str) -> HubConfig:
    scenes_path = os.path.join(os.path.dirname(manifest_path), "scenes.yaml")
    if not os.path.exists(scenes_path):
        return cfg
    with open(scenes_path, "r", encoding="utf-8") as fh:
        scenes = yaml.safe_load(fh) or {}
    active = scenes.get(scene)
    if active is None:
        return cfg
    if "sources" in active:
        cfg.sources = parse_sources(active["sources"] or [])
    if "moods" in active:
        cfg.moods = [MoodRule.model_validate(m) for m in (active["moods"] or [])]
    if "alerts" in active:
        cfg.alerts = [AlertRule.model_validate(a) for a in (active["alerts"] or [])]
    return cfg
