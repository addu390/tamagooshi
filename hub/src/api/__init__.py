from .agents import router as agents_router
from .brands import router as brands_router
from .config import router as config_router
from .connection import router as connection_router
from .events import router as events_router
from .rules import router as rules_router
from .secrets import router as secrets_router
from .sources import router as sources_router
from .status import router as status_router
from .ui import mount_ui

__all__ = [
    "agents_router", "brands_router", "config_router", "connection_router", "events_router",
    "rules_router", "secrets_router", "sources_router", "status_router", "mount_ui",
]
