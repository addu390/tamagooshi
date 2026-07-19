import os

spec_dir = os.path.abspath(SPECPATH)
repo = os.path.abspath(os.path.join(spec_dir, "..", ".."))
hub = os.path.join(repo, "hub")

a = Analysis(
    [os.path.join(hub, "src", "__main__.py")],
    pathex=[hub],
    datas=[
        (os.path.join(repo, "brands"), "brands"),
        (os.path.join(repo, "app", "ui"), "ui"),
        (os.path.join(repo, "docs", "css", "tokens.css"), "ui"),
    ],
    hiddenimports=["uvicorn.protocols.http.auto", "uvicorn.protocols.websockets.auto",
                   "uvicorn.lifespan.on"],
    excludes=["tkinter"],
)

pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    exclude_binaries=True,
    name="hubd",
    console=True,
)

coll = COLLECT(exe, a.binaries, a.datas, name="hubd")
