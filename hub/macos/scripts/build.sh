#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD="$ROOT/build"
VERSION="${VERSION:-0.1.0}"
VENV="$BUILD/venv"

mkdir -p "$BUILD"

if [ ! -x "$VENV/bin/python" ]; then
  python3 -m venv "$VENV"
fi
"$VENV/bin/pip" install --quiet --upgrade pip pyinstaller
"$VENV/bin/pip" install --quiet "$ROOT/hub/backend"

"$VENV/bin/pyinstaller" "$SCRIPT_DIR/hubd.spec" \
  --distpath "$BUILD/dist" --workpath "$BUILD/work" --noconfirm

swift build -c release --package-path "$ROOT/hub/macos"
BIN="$(swift build -c release --package-path "$ROOT/hub/macos" --show-bin-path)/TamagooshiApp"

APP="$BUILD/Tamagooshi.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

cp "$BIN" "$APP/Contents/MacOS/TamagooshiApp"
cp "$ROOT/hub/macos/Info.plist" "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $VERSION" "$APP/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $VERSION" "$APP/Contents/Info.plist"
cp -R "$BUILD/dist/hubd" "$APP/Contents/Resources/hubd"

IDENTITY="-"
if security find-identity -v -p codesigning 2>/dev/null | grep -q "Developer ID Application"; then
  IDENTITY="$(security find-identity -v -p codesigning | grep "Developer ID Application" | head -1 | awk -F'"' '{print $2}')"
fi
echo "signing with: $IDENTITY"
xattr -cr "$APP" 2>/dev/null || true
codesign --deep --force --options runtime --sign "$IDENTITY" "$APP"
codesign --verify --deep --strict "$APP"

echo "built $APP (v$VERSION)"
