from __future__ import annotations

import os

from .settings import data_dir


def _secrets_path() -> str:
    return os.path.join(data_dir(), "secrets.env")


def load_secrets() -> dict[str, str]:
    secrets: dict[str, str] = {}
    try:
        with open(_secrets_path(), "r", encoding="utf-8") as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith("#") or "=" not in line:
                    continue
                name, _, value = line.partition("=")
                secrets[name.strip()] = value.strip()
    except FileNotFoundError:
        pass
    return secrets


def _write(secrets: dict[str, str]) -> None:
    os.makedirs(data_dir(), exist_ok=True)
    path = _secrets_path()
    with open(path, "w", encoding="utf-8") as fh:
        fh.writelines(f"{name}={value}\n" for name, value in sorted(secrets.items()))
    os.chmod(path, 0o600)


def set_secret(name: str, value: str) -> None:
    secrets = load_secrets()
    secrets[name] = value
    _write(secrets)
    os.environ[name] = value


def delete_secret(name: str) -> None:
    secrets = load_secrets()
    if secrets.pop(name, None) is not None:
        _write(secrets)
    os.environ.pop(name, None)


def apply_secrets() -> None:
    for name, value in load_secrets().items():
        os.environ.setdefault(name, value)
