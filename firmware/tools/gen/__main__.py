import json
import os
import sys


def _paths():
    here = os.path.dirname(os.path.abspath(__file__))
    firmware = os.path.abspath(os.path.join(here, "..", ".."))
    repo = os.path.abspath(os.path.join(firmware, ".."))
    return firmware, repo


def _brand(args):
    from gen.pipeline import generate

    firmware, repo = _paths()
    brand = args[0] if args else os.environ.get("TAMA_BRAND", "gooshi")
    out = args[1] if len(args) > 1 else os.path.join(firmware, ".gen", "current")
    generate(brand, os.path.join(repo, "brands"), out, os.environ.get("TAMA_DEV", ""))
    print(f"generated {brand} -> {out}")


def _catalog(args):
    from gen.emit.catalog import catalog, render

    _, repo = _paths()
    data = catalog()
    targets = (args if args else
               [os.path.join(repo, "website", "docs", "js", "gen", "catalog.js"),
                os.path.join(repo, "hub", "console", "js", "catalog.gen.js")])

    for out in targets:
        os.makedirs(os.path.dirname(out), exist_ok=True)
        with open(out, "w", encoding="utf-8") as fh:
            fh.write(render(data))
        print(f"wrote catalog -> {out}")

    if not args:
        _sync([])


def _sync(args):
    from gen.emit.mirror import stale, sync

    _, repo = _paths()
    if "--check" in args:
        names = stale(repo)
        if names:
            raise SystemExit(f"website wire mirror stale: {', '.join(names)} "
                             "(run: python3 -m gen sync)")
        print("website wire mirror up to date")
        return

    for path in sync(repo):
        print(f"mirrored wire module -> {path}")


def _matrix(args):
    from gen.platform.boards import ci_matrix

    print(json.dumps({"include": ci_matrix()}))


def _flash(args):
    from gen.platform.boards import flash_catalog

    print(json.dumps(flash_catalog(), indent=2))


def _new(args):
    from gen.scaffold import new

    new(args)


def _blob(args):
    from gen.emit.blob import encode, from_brand
    from gen.manifest import load

    if len(args) != 2:
        raise SystemExit("usage: python3 -m gen blob <brand-config.yaml> <out.bin>")
    config = from_brand(load(args[0]), os.path.dirname(os.path.abspath(args[0])))
    blob = encode(config)
    with open(args[1], "wb") as fh:
        fh.write(blob)
    print(f"wrote {len(blob)}B config blob -> {args[1]}")


COMMANDS = {"brand": _brand, "catalog": _catalog, "matrix": _matrix, "flash": _flash,
            "blob": _blob, "sync": _sync, "new": _new}


def main(argv):
    cmd = argv[1] if len(argv) > 1 else ""
    if cmd not in COMMANDS:
        raise SystemExit(f"usage: python3 -m gen {{{'|'.join(COMMANDS)}}} [args]")
    COMMANDS[cmd](argv[2:])


main(sys.argv)
