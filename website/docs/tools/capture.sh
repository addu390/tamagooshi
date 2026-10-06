#!/usr/bin/env bash
set -euo pipefail

SCENE="${1:-mood}"
case "$SCENE" in
  claude-*) BRAND="${BRAND:-claude}" ;;
  *)        BRAND="${BRAND:-demo}" ;;
esac
FPS="${FPS:-15}"
STRIDE="${STRIDE:-8}"

INPUT=""
case "$SCENE" in
  mood)
    START="${START:-home}"; N="${N:-300}"; WARMUP="${WARMUP:-60}"
    ;;
  claude-home)
    START="${START:-home}"; N="${N:-200}"; WARMUP="${WARMUP:-60}"
    ;;
  claude-work)
    START="${START:-menu}"; N="${N:-250}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-800:next,1250:next,1700:next,2150:next,2700:select,3900:select,7100:select,12100:select}"
    ;;
  *)
    START="${START:-home}"; N="${N:-300}"; WARMUP="${WARMUP:-60}"
    ;;
esac

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/website/docs/assets/videos"
FRAMES="$(mktemp -d)"
trap 'rm -rf "$FRAMES"' EXIT

cd "$ROOT/firmware"
TAMA_BRAND="$BRAND" pio run -e native_sim >/dev/null

TAMA_START="$START" TAMA_INPUT="$INPUT" \
  TAMA_CAP_DIR="$FRAMES" TAMA_CAP_N="$N" TAMA_CAP_STRIDE="$STRIDE" TAMA_CAP_WARMUP="$WARMUP" \
  ./.pio/build/native_sim/program >/dev/null 2>&1 || true

mkdir -p "$OUT"

GAMEWIN="$(python3 "$ROOT/website/docs/tools/normframes.py" "$FRAMES")"
echo "rotated frame window (first last total): $GAMEWIN"

SCALE="scale=270:480:flags=neighbor"
ffmpeg -y -framerate "$FPS" -i "$FRAMES/frame_%04d.ppm" -vf "$SCALE" \
  -c:v libvpx-vp9 -pix_fmt yuv420p -b:v 0 -crf 32 "$OUT/$SCENE.webm" >/dev/null 2>&1
ffmpeg -y -framerate "$FPS" -i "$FRAMES/frame_%04d.ppm" -vf "$SCALE" \
  -c:v libx264 -pix_fmt yuv420p -crf 24 -movflags +faststart "$OUT/$SCENE.mp4" >/dev/null 2>&1

echo "captured $(ls "$FRAMES" | wc -l | tr -d ' ') frames -> $OUT/$SCENE.{webm,mp4}"
