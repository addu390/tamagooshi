import { frameSrc, partById, partSources } from "./pack.js";

const hexRgb = (hex) => [1, 3, 5].map((i) => parseInt(hex.slice(i, i + 2), 16));

const luminance = ([r, g, b]) => (0.299 * r + 0.587 * g + 0.114 * b) / 255;

function rgbToHsl([r, g, b]) {
  r /= 255; g /= 255; b /= 255;
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const l = (max + min) / 2;
  if (max === min) return [0, 0, l];

  const d = max - min;
  const s = d / (l > 0.5 ? 2 - max - min : max + min);
  const h = max === r ? ((g - b) / d + (g < b ? 6 : 0)) / 6
          : max === g ? ((b - r) / d + 2) / 6
          : ((r - g) / d + 4) / 6;
  return [h, s, l];
}

function hslToRgb(h, s, l) {
  const hue = (t) => {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1 / 6) return p + (q - p) * 6 * t;
    if (t < 1 / 2) return q;
    if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
    return p;
  };
  const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
  const p = 2 * l - q;
  return [hue(h + 1 / 3), hue(h), hue(h - 1 / 3)].map((v) => Math.round(v * 255));
}

function tint(img, targetHex, refHex, only) {
  const canvas = document.createElement("canvas");
  canvas.width = img.width;
  canvas.height = img.height;

  const ctx = canvas.getContext("2d");
  ctx.drawImage(img, 0, 0);

  const [th, ts, tl] = rgbToHsl(hexRgb(targetHex));
  const refLum = luminance(hexRgb(refHex));
  const onlySet = only && new Set(only.map((h) => hexRgb(h).join(",")));
  const data = ctx.getImageData(0, 0, canvas.width, canvas.height);
  const px = data.data;

  for (let i = 0; i < px.length; i += 4) {
    if (!px[i + 3]) continue;
    if (onlySet && !onlySet.has(`${px[i]},${px[i + 1]},${px[i + 2]}`)) continue;
    const lum = luminance([px[i], px[i + 1], px[i + 2]]);
    const [r, g, b] = hslToRgb(th, ts, Math.min(1, tl * (lum / refLum)));
    px[i] = r; px[i + 1] = g; px[i + 2] = b;
  }

  ctx.putImageData(data, 0, 0);
  return canvas;
}

function loadImage(src) {
  return new Promise((resolve, reject) => {
    const img = new Image();
    img.onload = () => resolve(img);
    img.onerror = () => reject(new Error(`failed to load part image: ${src}`));
    img.src = src;
  });
}

export async function createRenderer(pack) {
  const sources = new Set();
  pack.slots.forEach((slot) => slot.parts.forEach((part) => {
    partSources(part).forEach((s) => sources.add(s));
  }));

  const images = new Map();
  await Promise.all([...sources].map(async (rel) => {
    images.set(rel, await loadImage(new URL(rel, pack.assetBase)));
  }));

  const tints = new Map();
  const refOf = (paletteId) => pack.palettes.find((p) => p.id === paletteId).ref;

  const layer = (rel, slot, color) => {
    if (!slot.palette || !color) return images.get(rel);

    const key = `${rel}|${color}`;
    if (!tints.has(key)) {
      tints.set(key, tint(images.get(rel), color, refOf(slot.palette), slot.tintOnly));
    }
    return tints.get(key);
  };

  const drawCentered = (ctx, img, { at: [cx, cy], mirror, flip }) => {
    const paint = () => ctx.drawImage(img, cx - img.width / 2, cy - img.height / 2);
    if (!flip) paint();

    if (flip || mirror != null) {
      ctx.save();
      ctx.translate(2 * (flip ? cx : mirror), 0);
      ctx.scale(-1, 1);
      paint();
      ctx.restore();
    }
  };

  const drawSlots = (ctx, slots, selection, frame) => {
    slots.forEach((slot) => {
      const part = partById(slot, selection.parts[slot.id]);
      if (!part) return;

      const img = layer(frameSrc(part, frame), slot, selection.colors[slot.palette]);
      drawCentered(ctx, img, slot);
    });
  };

  function renderLayer(slots, selection, frame, scale) {
    const canvas = document.createElement("canvas");
    canvas.width = pack.canvas.w * scale;
    canvas.height = pack.canvas.h * scale;

    const ctx = canvas.getContext("2d");
    ctx.imageSmoothingEnabled = !pack.pixel;
    ctx.imageSmoothingQuality = "high";
    ctx.scale(scale, scale);
    drawSlots(ctx, slots, selection, frame);
    return canvas;
  }

  function silhouette(figure, color) {
    const canvas = document.createElement("canvas");
    canvas.width = figure.width;
    canvas.height = figure.height;

    const ctx = canvas.getContext("2d");
    ctx.drawImage(figure, 0, 0);
    ctx.globalCompositeOperation = "source-in";
    ctx.fillStyle = color;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    return canvas;
  }

  function drawFrame(ctx, selection, frame, scale = 1) {
    const { w, h } = pack.canvas;
    ctx.save();
    ctx.imageSmoothingEnabled = !pack.pixel;
    ctx.imageSmoothingQuality = "high";
    ctx.scale(scale, scale);
    ctx.clearRect(0, 0, w, h);

    drawSlots(ctx, pack.slots.filter((s) => s.backdrop), selection, frame);
    const figureSlots = pack.slots.filter((s) => !s.backdrop);

    if (selection.outline && pack.outline) {
      const figure = renderLayer(figureSlots, selection, frame, scale);
      const ink = silhouette(figure, pack.outline.color);
      const o = pack.outline.width;

      for (const [dx, dy] of [[o, 0], [-o, 0], [0, o], [0, -o], [o, o], [o, -o], [-o, o], [-o, -o]]) {
        ctx.drawImage(ink, dx, dy, w, h);
      }
      ctx.drawImage(figure, 0, 0, w, h);
    } else {
      drawSlots(ctx, figureSlots, selection, frame);
    }

    ctx.restore();
  }

  return { drawFrame, partImage: (rel) => images.get(rel) };
}
