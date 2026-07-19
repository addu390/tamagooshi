LINKS = ("ble", "wifi")
LINK_MACRO = {"ble": "TAMA_ENABLE_BLE", "wifi": "TAMA_ENABLE_WIFI"}
PROTOCOLS = {"ble": ("gatt",), "wifi": ("mqtt",)}
DEFAULT_PROTOCOL = {"ble": "gatt", "wifi": "mqtt"}
PROTOCOL_MACRO = {"gatt": "TAMA_PROTO_GATT", "mqtt": "TAMA_PROTO_MQTT"}


def transport_macros(spec):
    hub = spec.get("wifi") or ("gatt" if "ble" in spec else None)
    if hub is None:
        raise SystemExit("transports need a bearer that can carry the hub")
    macros = {LINK_MACRO[link] for link in spec}
    macros.add(PROTOCOL_MACRO[hub])
    return sorted(macros)
