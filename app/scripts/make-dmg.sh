#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD="$ROOT/build"
VERSION="${VERSION:-0.1.0}"
APP="$BUILD/Tamagooshi.app"
DMG="$BUILD/Tamagooshi-$VERSION.dmg"

[ -d "$APP" ] || { echo "run build-app.sh first"; exit 1; }

STAGING="$BUILD/.dmg-staging"
rm -rf "$STAGING" "$DMG"
mkdir -p "$STAGING"
cp -R "$APP" "$STAGING/"
ln -s /Applications "$STAGING/Applications"

hdiutil create -volname "Tamagooshi" -srcfolder "$STAGING" -ov -format UDZO "$DMG"
rm -rf "$STAGING"

echo "built $DMG"
