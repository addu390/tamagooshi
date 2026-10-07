from .brands import router as brands_router
from .config import router as config_router
from .connection import router as connection_router
from .events import router as events_router
from .flash import router as flash_router
from .rules import router as rules_router
from .status import router as status_router

__all__ = [
    "brands_router", "config_router", "connection_router", "events_router",
    "flash_router", "rules_router", "status_router",
]
