import os
import sys

Import("env")

project = env["PROJECT_DIR"]
generator = os.path.join(project, "tools", "generator")
brands = os.path.abspath(os.path.join(project, "..", "brands"))
brand = os.environ.get("TAMA_BRAND", "gooshi")
out = os.path.join(project, ".gen", "current")

try:
    import PIL  # noqa: F401
    import yaml  # noqa: F401
except ImportError:
    env.Execute('"$PYTHONEXE" -m pip install -r "%s"' % os.path.join(generator, "requirements.txt"))

sys.path.insert(0, generator)
import build

macros = build.generate(brand, brands, out, os.environ.get("TAMA_DEV", ""))
env.Append(CPPPATH=[out])

if env["PIOPLATFORM"] != "native":
    override = os.environ.get("TAMA_TRANSPORTS")
    if override:
        macros = build.transport_macros(build.parse_transports(override))
    env.Append(CPPDEFINES=macros)
