from __future__ import annotations

import logging
from typing import Optional

from pydantic import ValidationError

from .config import HubConfig
from .domain import AlertEngine, MetricStore, MetricUpdate, MoodEngine
from .messaging import Publisher
from .wire import protocol

log = logging.getLogger("tamagooshi.pipeline")


class Pipeline:
    def __init__(self, config: HubConfig, publisher: Publisher):
        self._config = config
        self._publisher = publisher
        self._store = MetricStore()
        self._mood = MoodEngine(config.moods, config.default_mood)
        self._alerts = AlertEngine(config.alerts)
        self._current_mood: Optional[str] = None

    @property
    def alerts(self) -> AlertEngine:
        return self._alerts

    @property
    def current_mood(self) -> Optional[str]:
        return self._current_mood

    def metric_snapshot(self):
        return self._store.snapshot()

    def announce(self) -> None:
        self._publisher.publish_branding(self._config.brand)
        if self._config.brand.theme or self._config.brand.mascot:
            self._publisher.publish_config(self._config.brand)
        self._publisher.publish_time(self._config.brand.tz_offset)
        self._publish_mood(self._config.default_mood)

    async def emit(self, update: MetricUpdate) -> None:
        self._store.update(update)
        log.info("metric %s=%s (%s)", update.key, update.value, update.trend)
        self._publisher.publish_metric(update)

        transition = self._alerts.evaluate(self._store)
        for rule in transition.raised:
            log.warning("alert RAISED %s: %s", rule.id, rule.title)
            self._publisher.publish_page(rule)
        for page_id in transition.cleared:
            log.info("alert CLEARED %s", page_id)
            self._publisher.clear_page(page_id)

        self._publish_mood(self._mood.evaluate(self._store))

    def replay(self) -> None:
        log.info("device announced; replaying state")
        self._publisher.publish_branding(self._config.brand)
        if self._config.brand.theme or self._config.brand.mascot:
            self._publisher.publish_config(self._config.brand)
        self._publisher.publish_time(self._config.brand.tz_offset)
        if self._current_mood:
            self._publisher.publish_mood(self._current_mood)
        for update in self._store.last():
            self._publisher.publish_metric(update)
        for rule in self._alerts.active_rules():
            self._publisher.publish_page(rule)

    def acknowledge(self, env: protocol.Envelope) -> None:
        try:
            ack = protocol.PageAck.model_validate(env.body)
        except ValidationError:
            return
        if ack.page_id:
            log.info("ack received for %s", ack.page_id)
            self._alerts.acknowledge(ack.page_id)

    def _publish_mood(self, mood: str, reason: Optional[str] = None) -> None:
        if mood == self._current_mood:
            return
        self._current_mood = mood
        log.info("mood -> %s", mood)
        self._publisher.publish_mood(mood, reason)
