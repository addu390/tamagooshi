export async function loadPack(url) {
  const res = await fetch(url);
  if (!res.ok) throw new Error(`failed to load pack: ${url}`);

  const pack = await res.json();
  pack.assetBase = new URL(pack.assets, new URL(url, location.href));
  pack.slots.forEach((slot) => {
    if (typeof slot.parts === "string") slot.parts = pack.partSets[slot.parts];
  });
  return pack;
}

export const partSources = (part) =>
  typeof part.src === "string" ? [part.src] : Object.values(part.src);

export const frameSrc = (part, frame) =>
  typeof part.src === "string" ? part.src : part.src[frame] || partSources(part)[0];

export const partById = (slot, id) => slot.parts.find((p) => p.id === id) || null;

export function defaultSelection(pack) {
  const parts = {};
  pack.slots.forEach((slot) => {
    parts[slot.id] = slot.default === "none" ? null : slot.default || slot.parts[0].id;
  });

  const colors = {};
  (pack.palettes || []).forEach((pal) => { colors[pal.id] = pal.colors[0]; });
  return { parts, colors, outline: false };
}
