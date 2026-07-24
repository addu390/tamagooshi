import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "firmware" / "tools"))

from gen import manifest as gen_manifest
from gen.emit import blob as gen_blob
from gen.emit.mirror import stale
from gen.network.transports import DEFAULT_PROTOCOL, LINKS, PROTOCOLS
from gen.platform.boards import RELEASE_BASE
from src.api.routes.flash import RELEASE_PREFIX
from src.config import loader
from src.config.sources.fs import manifest_candidates
from src.network.transport.factory import TRANSPORTS


def _catalog():
    text = (REPO / "hub" / "console" / "js" / "catalog.gen.js").read_text(encoding="utf-8")
    return json.loads(text[text.index("{"):text.rindex("}") + 1])


@pytest.mark.parametrize("value", [None, "", "+05:30", "-05:00", "5", "-2", "0:45",
                                   120, -90, "+14:00", 5.9])
def test_tz_minutes_parity(value):
    assert loader._tz_minutes(value) == gen_manifest.tz_minutes(value)


@pytest.mark.parametrize("brand_id", ["gooshi", "acme"])
def test_brand_candidate_parity(brand_id):
    hub = [os.path.relpath(p, "root") for p in manifest_candidates("root", brand_id)]
    gen = [os.path.relpath(p, "root")
           for p in gen_manifest.manifest_candidates("root", brand_id)]
    assert hub == gen == [os.path.join(brand_id, "config.yaml"), f"{brand_id}.yaml"]


def test_transport_keys_parity():
    assert set(TRANSPORTS) == set(LINKS)
    for link, protocols in TRANSPORTS.items():
        assert set(protocols) == set(PROTOCOLS[link])
        assert next(iter(protocols)) == DEFAULT_PROTOCOL[link]

    catalog = _catalog()["transports"]
    assert set(TRANSPORTS) == {link for link, _ in catalog["links"]}
    for link, protocols in TRANSPORTS.items():
        assert set(protocols) == {p for p, _ in catalog["protocols"][link]}


def test_release_prefix_parity():
    assert RELEASE_PREFIX == RELEASE_BASE == _catalog()["release"]


def test_docs_wire_mirror_fresh():
    assert stale(str(REPO)) == []


def test_blob_byte_parity():
    node = shutil.which("node")
    if node is None:
        pytest.skip("node not installed")

    manifest = {
        "brand": {"name": "ACME", "tagline": "beep", "website": "acme.dev", "mascot": "AC"},
        "device": {
            "timezone": "+05:30",
            "theme": {
                "custom": [{"name": "lava", "colors": {"surface": "#1a0e0e", "ink": "#ffb0a0",
                                                       "accent": "#ff5533"}}],
                "default": "lava",
                "enabled": ["midnight", "lava"],
            },
            "typeface": {"default": "mono", "enabled": "all"},
            "mascot": {"default": "cat", "mood": "happy", "enabled": ["pals"]},
            "games": {"enabled": ["pong"]},
            "apps": {"enabled": "all"},
        },
    }

    script = (f"import({json.dumps((REPO / 'hub/console/js/wire/blob.js').as_uri())})"
              f".then((m) => process.stdout.write(m.encode(m.fromManifest("
              f"{json.dumps(manifest)}))));")
    out = subprocess.run([node, "-e", script], capture_output=True, check=True).stdout

    assert out == gen_blob.encode(gen_blob.from_brand(manifest))
