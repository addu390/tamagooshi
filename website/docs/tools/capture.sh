#!/usr/bin/env bash
set -euo pipefail

SCENE="${1:-mood}"
case "$SCENE" in
  claude-*) BRAND="${BRAND:-claude}" ;;
  *)        BRAND="${BRAND:-demo}" ;;
esac
DEV="${DEV:-John}"
BROKER="${BROKER:-localhost:1883}"
FPS="${FPS:-15}"
STRIDE="${STRIDE:-8}"

HUBSCENE=""
INPUT=""
BUDDY=""
HUB=1
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
    START="${START:-menu}"; N="${N:-505}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-800:next,1250:next,1700:next,2150:next,2700:select,14600:next,15100:next,15500:next,15900:next,16300:next,16700:next,17200:select,18400:next,19400:select,22600:select,27600:select}"
    ;;
  alerts)
    HUBSCENE="alerts"; BUDDY="approve"
    START="${START:-buddy}"; N="${N:-300}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-4200:select,8000:next,9200:select,14000:select}"
    ;;
  brand)
    HUBSCENE="brand"
    START="${START:-home}"; N="${N:-290}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-3000:select,3500:next,4000:select,4600:select,5400:select,6200:select,7000:select,8600:home,9600:select,10100:next,10600:next,11100:select,11600:next,12100:select,12600:select,13200:select,14400:back,14900:next,15400:select,16000:select,16600:select,17200:select,18400:home}"
    ;;
  claude-home)
    HUB=0
    START="${START:-home}"; N="${N:-200}"; WARMUP="${WARMUP:-60}"
    ;;
  claude-work)
    HUB=0; BUDDY="work"
    START="${START:-menu}"; N="${N:-485}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-800:next,1250:next,1700:next,2150:next,2700:select,14600:next,15100:next,15500:next,15900:next,16300:next,16700:next,17200:select,18400:select,21600:select,26600:select}"
    ;;
  claude-approve)
    HUB=0; BUDDY="approve"
    START="${START:-buddy}"; N="${N:-220}"; WARMUP="${WARMUP:-60}"
    INPUT="${INPUT:-4200:select,8000:next,9200:select}"
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
TAMA_BRAND="$BRAND" TAMA_DEV="$DEV" pio run -e native_sim >/dev/null

if [ "$HUB" = "1" ]; then
  cd "$ROOT"
  TAMA_BRAND="$BRAND" TAMA_SCENE="$HUBSCENE" docker compose up -d --build broker hub >/dev/null
  TAMA_BRAND="$BRAND" TAMA_SCENE="$HUBSCENE" docker compose up -d --force-recreate hub >/dev/null
  sleep 2
else
  BROKER=""
fi

cd "$ROOT/firmware"
TAMA_START="$START" TAMA_BROKER="$BROKER" TAMA_INPUT="$INPUT" TAMA_BUDDY="$BUDDY" \
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
