const exportScale = (pack) => (pack.pixel ? 1 : 2);

export function renderFrame(renderer, pack, selection, frame, scale = exportScale(pack)) {
  const canvas = document.createElement("canvas");
  canvas.width = pack.canvas.w * scale;
  canvas.height = pack.canvas.h * scale;
  renderer.drawFrame(canvas.getContext("2d"), selection, frame, scale);
  return canvas;
}

export function renderSheet(renderer, pack, selection, scale = exportScale(pack)) {
  const canvas = document.createElement("canvas");
  canvas.width = pack.canvas.w * pack.frames.length * scale;
  canvas.height = pack.canvas.h * scale;

  const ctx = canvas.getContext("2d");
  pack.frames.forEach((frame, i) =>
    ctx.drawImage(renderFrame(renderer, pack, selection, frame, scale), i * pack.canvas.w * scale, 0));

  return canvas;
}

export function downloadCanvas(canvas, filename) {
  canvas.toBlob((blob) => {
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = filename;
    a.click();
    URL.revokeObjectURL(a.href);
  }, "image/png");
}

export function mascotId(name) {
  const id = name.trim().toLowerCase().replace(/[^a-z0-9]+/g, "_").replace(/^_+|_+$/g, "");
  if (!id) return "my_mascot";
  return /^[0-9]/.test(id) ? `m${id}` : id;
}

export const mascotYaml = (name) => {
  const id = mascotId(name);
  return [
    "mascot:",
    "  enabled: [custom]",
    "  custom:",
    `    - id: ${id}`,
    `      label: ${name.trim() || "My mascot"}`,
    "      category: custom",
    `      source: ${id}.png`,
  ].join("\n");
};

export const mascotTree = (name) => [
  "brands/",
  "  my-brand/",
  "    config.yaml",
  `    ${mascotId(name)}.png`,
].join("\n");
