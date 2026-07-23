import importlib
import os
import sys
from pathlib import Path

Import("env")

project = env["PROJECT_DIR"]
tools = os.path.join(project, "tools")
brands = os.path.abspath(os.path.join(project, "..", "brands"))
brand = os.environ.get("TAMA_BRAND", "gooshi")
out = os.path.join(project, ".gen", "current")


def build_python():
    for entry in sys.path:
        path = Path(entry)
        if path.name != "site-packages":
            continue
        for candidate in (path.parents[2] / "bin" / "python", path.parents[1] / "Scripts" / "python.exe"):
            if candidate.exists():
                return str(candidate)
    return sys.executable


try:
    import PIL  # noqa: F401
    import yaml  # noqa: F401
except ImportError:
    env.Execute(
        '"%s" -m pip --python "%s" install -r "%s"'
        % (sys.executable, build_python(), os.path.join(tools, "requirements.txt"))
    )
    importlib.invalidate_caches()

sys.path.insert(0, tools)
from gen.manifest import parse_transports
from gen.network.transports import transport_macros
from gen.pipeline import generate

macros = generate(brand, brands, out, os.environ.get("TAMA_DEV", ""))
env.Append(CPPPATH=[out])

if env["PIOPLATFORM"] != "native":
    override = os.environ.get("TAMA_TRANSPORTS")
    if override:
        macros = transport_macros(parse_transports(override))
    env.Append(CPPDEFINES=macros)
