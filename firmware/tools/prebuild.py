import os
import sys

Import("env")

project = env["PROJECT_DIR"]
tools = os.path.join(project, "tools")
brands = os.path.abspath(os.path.join(project, "..", "brands"))
brand = os.environ.get("TAMA_BRAND", "gooshi")
out = os.path.join(project, ".gen", "current")

try:
    import PIL  # noqa: F401
    import yaml  # noqa: F401
except ImportError:
    env.Execute('"$PYTHONEXE" -m pip install -r "%s"' % os.path.join(tools, "requirements.txt"))

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
