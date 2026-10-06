from __future__ import annotations

import logging

from ..config import HubConfig
from ..network import Publisher

log = logging.getLogger("tamagooshi.sync")


class DeviceSync:
    def __init__(self, config: HubConfig, publisher: Publisher):
        self._config = config
        self._publisher = publisher

    def announce(self) -> None:
        brand = self._config.brand
        self._publisher.publish_branding(brand)
        if brand.theme or brand.mascot:
            self._publisher.publish_config(brand)
        self._publisher.publish_time(brand.tz_offset)

    def replay(self) -> None:
        log.info("device announced; replaying state")
        self.announce()
