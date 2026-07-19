def metric(device_id: str, key: str) -> str:
    return f"devices/{device_id}/metrics/{key}"


def branding(device_id: str) -> str:
    return f"devices/{device_id}/branding"


def mood(device_id: str) -> str:
    return f"devices/{device_id}/mood"


def config(device_id: str) -> str:
    return f"devices/{device_id}/config"


def time(device_id: str) -> str:
    return f"devices/{device_id}/time"


def pages(device_id: str) -> str:
    return f"devices/{device_id}/pages"


def acks(device_id: str) -> str:
    return f"devices/{device_id}/acks"


def status(device_id: str) -> str:
    return f"devices/{device_id}/status"


def hello(device_id: str) -> str:
    return f"devices/{device_id}/hello"


def device_wildcard(device_id: str) -> str:
    return f"devices/{device_id}/#"
