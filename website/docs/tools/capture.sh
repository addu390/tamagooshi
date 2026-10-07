#!/usr/bin/env bash
set -euo pipefail

SCENE="${1:-mood}"
BRAND="${BRAND:-gooshi}"
FPS="${FPS:-15}"
STRIDE="${STRIDE:-8}"
WARMUP="${WARMUP:-60}"
AGENT="${AGENT:-}"
MUSE="${MUSE:-}"

# A scene is one or more segments, each "start-screen:frames:input-script".
# Segments are captured back to back and encoded as one clip.
case "$SCENE" in
  brand)
    SEGMENTS=("home:180:600:next,1000:next,1400:next,1800:select,2200:select,2700:select,3908:select,4916:select,6004:select,6600:back,7000:home,7400:next,7800:next,8100:next,8400:next,8700:select,9000:next,9300:select,9700:next,10200:select,10700:select,11200:select,11700:select,12200:back,12600:back,13000:back")
    ;;
  mood)
    AGENT="online"
    MUSE="1500:publish:build:BUILD:98%:star,3000:mood:celebrate,5500:publish:build:BUILD:71%:star,6500:mood:sick,8500:say:ci is red. on it"
    SEGMENTS=("home:150:")
    ;;
  agents)
    AGENT="online"
    SEGMENTS=("menu:150:2000:select,3000:select,5000:select")
    ;;
  apps)
    SEGMENTS=("apps:150:800:next,1400:select,2000:select,3600:next,5000:select,5600:back,6200:next,6600:next,7000:next,7600:select,12400:back")
    ;;
  games)
    SEGMENTS=("play:150:600:next,1000:select,1600:select,2016:select,2544:select,3056:select,3616:select,4128:select,4656:select,5184:select,5344:select,5872:select,6384:select,7136:select,7680:select,8160:select,8592:select,9088:select,9616:select,9904:select,10080:select,10176:select,10656:select,11200:select,12000:select")
    ;;
  *)
    SEGMENTS=("home:300:")
    ;;
esac

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OUT="$ROOT/website/docs/assets/videos"
FRAMES="$(mktemp -d)"
trap 'rm -rf "$FRAMES"' EXIT

cd "$ROOT/firmware"
TAMA_BRAND="$BRAND" pio run -e native_sim >/dev/null

total=0
for seg in "${SEGMENTS[@]}"; do
  start="${seg%%:*}"; rest="${seg#*:}"
  n="${rest%%:*}"; input="${rest#*:}"
  dir="$(mktemp -d)"
  TAMA_START="$start" TAMA_INPUT="$input" TAMA_AGENT="$AGENT" TAMA_MUSE="$MUSE" \
    TAMA_CAP_DIR="$dir" TAMA_CAP_N="$n" TAMA_CAP_STRIDE="$STRIDE" TAMA_CAP_WARMUP="$WARMUP" \
    ./.pio/build/native_sim/program >/dev/null 2>&1 || true
  for f in "$dir"/frame_*.ppm; do
    mv "$f" "$(printf '%s/frame_%04d.ppm' "$FRAMES" "$total")"
    total=$((total + 1))
  done
  rm -rf "$dir"
done

mkdir -p "$OUT"

GAMEWIN="$(python3 "$ROOT/website/docs/tools/normframes.py" "$FRAMES")"
echo "rotated frame window (first last total): $GAMEWIN"

SCALE="scale=270:480:flags=neighbor"
ffmpeg -y -framerate "$FPS" -i "$FRAMES/frame_%04d.ppm" -vf "$SCALE" \
  -c:v libvpx-vp9 -pix_fmt yuv420p -b:v 0 -crf 32 "$OUT/$SCENE.webm" >/dev/null 2>&1
ffmpeg -y -framerate "$FPS" -i "$FRAMES/frame_%04d.ppm" -vf "$SCALE" \
  -c:v libx264 -pix_fmt yuv420p -crf 24 -movflags +faststart "$OUT/$SCENE.mp4" >/dev/null 2>&1

echo "captured $total frames -> $OUT/$SCENE.{webm,mp4}"
