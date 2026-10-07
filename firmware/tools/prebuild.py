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
    requirements = os.path.join(tools, "requirements.txt")
    env.Execute(
        f'"{sys.executable}" -m pip --python "{build_python()}" install -r "{requirements}"'
    )
    importlib.invalidate_caches()

def dotenv(key):
    if key in os.environ:
        return os.environ[key]
    path = os.path.join(project, "..", ".env")
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as fh:
        for line in fh:
            name, sep, value = line.strip().partition("=")
            if sep and name.strip() == key:
                return value.strip().strip("'\"")
    return None


sys.path.insert(0, tools)
from gen.pipeline import generate
from gen.platform.boards import BOARDS

board = env["PIOENV"] if env["PIOENV"] in BOARDS else None
macros = generate(brand, brands, out, os.environ.get("TAMA_TRANSPORTS"), board)
env.Append(CPPPATH=[out])

if env["PIOPLATFORM"] != "native":
    env.Append(CPPDEFINES=macros)
    if "TAMA_AGENT_MUSE" in macros:
        token = dotenv("MUSE_SDK_TOKEN")
        if not token:
            print("warning: MUSE_SDK_TOKEN is not set; Muse pairing will not report an SDK token")
        env.Append(CPPDEFINES=[("TAMA_MUSE_SDK_TOKEN", env.StringifyMacro(token or ""))])
