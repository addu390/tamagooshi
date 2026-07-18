#!/usr/bin/env bash
set -euo pipefail

SCENE="${1:-mood}"
BRAND="${BRAND:-demo}"
DEV="${DEV:-John}"
BROKER="${BROKER:-localhost:1883}"
FPS="${FPS:-15}"
STRIDE="${STRIDE:-8}"

HUBSCENE=""
INPUT=""
BUDDY=""
case "$SCENE" in
  mood)
    START="${START:-home}"; N="${N:-300}"; WARMUP="${WARMUP:-60}"
    ;;
  metrics)
    HUBSCENE="metrics"
    START="${START:-home}"; N="${N:-270}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-2400:select,3600:select,15600:home}"
    ;;
  agents)
    HUBSCENE="agents"; BUDDY="work"
    START="${START:-buddy}"; N="${N:-270}"; WARMUP="${WARMUP:-60}"
    ;;
  alerts)
    HUBSCENE="alerts"; BUDDY="approve"
    START="${START:-buddy}"; N="${N:-300}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-4200:select,8000:next,9200:select,14000:select}"
    ;;
  brand)
    HUBSCENE="brand"
    START="${START:-home}"; N="${N:-350}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-3200:select,4200:next,5100:select,6000:select,6800:select,8350:select,9800:home,12200:select,13200:next,14000:next,14800:select,15600:select,16600:next,17600:select,19200:select,20800:prev,21600:select,22600:next,23600:select,25200:select}"
    ;;
  *)
    START="${START:-home}"; N="${N:-300}"; WARMUP="${WARMUP:-60}"
    ;;
esac

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/docs/assets/videos"
FRAMES="$(mktemp -d)"
trap 'rm -rf "$FRAMES"' EXIT

cd "$ROOT/firmware"
TAMA_BRAND="$BRAND" TAMA_DEV="$DEV" pio run -e native_sim >/dev/null

cd "$ROOT"
TAMA_BRAND="$BRAND" TAMA_SCENE="$HUBSCENE" docker compose up -d --build broker hub >/dev/null
TAMA_BRAND="$BRAND" TAMA_SCENE="$HUBSCENE" docker compose up -d --force-recreate hub >/dev/null
sleep 2

cd "$ROOT/firmware"
TAMA_START="$START" TAMA_BROKER="$BROKER" TAMA_INPUT="$INPUT" TAMA_BUDDY="$BUDDY" \
  TAMA_CAP_DIR="$FRAMES" TAMA_CAP_N="$N" TAMA_CAP_STRIDE="$STRIDE" TAMA_CAP_WARMUP="$WARMUP" \
  ./.pio/build/native_sim/program >/dev/null 2>&1 || true

mkdir -p "$OUT"

GAMEWIN="$(python3 "$ROOT/docs/tools/normframes.py" "$FRAMES")"
echo "rotated frame window (first last total): $GAMEWIN"

SCALE="scale=270:480:flags=neighbor"
ffmpeg -y -framerate "$FPS" -i "$FRAMES/frame_%04d.ppm" -vf "$SCALE" \
  -c:v libvpx-vp9 -pix_fmt yuv420p -b:v 0 -crf 32 "$OUT/$SCENE.webm" >/dev/null 2>&1
ffmpeg -y -framerate "$FPS" -i "$FRAMES/frame_%04d.ppm" -vf "$SCALE" \
  -c:v libx264 -pix_fmt yuv420p -crf 24 -movflags +faststart "$OUT/$SCENE.mp4" >/dev/null 2>&1

echo "captured $(ls "$FRAMES" | wc -l | tr -d ' ') frames -> $OUT/$SCENE.{webm,mp4}"
