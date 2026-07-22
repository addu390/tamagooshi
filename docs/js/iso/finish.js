export function finishDefs(p) {
  const noise = (id, seed, ch, bias) => `<filter id="${p}${id}" x="0" y="0" width="100%" height="100%">
      <feTurbulence type="fractalNoise" baseFrequency="1" numOctaves="2" seed="${seed}" stitchTiles="stitch"/>
      <feColorMatrix type="matrix" values="0 0 0 0 ${ch}  0 0 0 0 ${ch}  0 0 0 0 ${ch}  1.6 0 0 0 ${bias}"/>
    </filter>`;
  return `<defs>
    <linearGradient id="${p}Sheen" x1="0" y1="0" x2="0.85" y2="1">
      <stop offset="0" stop-color="#ffffff" stop-opacity="0.4"/>
      <stop offset="0.35" stop-color="#ffffff" stop-opacity="0.1"/>
      <stop offset="0.6" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="1" stop-color="#000000" stop-opacity="0.12"/>
    </linearGradient>
    <linearGradient id="${p}Streak" x1="0" y1="0" x2="1" y2="0.4">
      <stop offset="0.28" stop-color="#ffffff" stop-opacity="0"/>
      <stop offset="0.4" stop-color="#ffffff" stop-opacity="0.25"/>
      <stop offset="0.5" stop-color="#ffffff" stop-opacity="0"/>
    </linearGradient>
    ${noise("GrainDark", 7, 0, -0.6)}
    ${noise("GrainLight", 31, 1, -0.7)}
    <filter id="${p}Shadow" x="-40%" y="-40%" width="180%" height="180%">
      <feGaussianBlur stdDeviation="3"/>
    </filter>
  </defs>`;
}

export const shadow = (p, points) =>
  `<polygon points="${points}" fill="#000" opacity="0.18" filter="url(#${p}Shadow)" transform="translate(5,4)"/>`;

export const sheen = (p, shape) =>
  shape(`fill="url(#${p}Sheen)"`) + shape(`fill="url(#${p}Streak)"`);

export const grain = (p, shape) =>
  shape(`filter="url(#${p}GrainDark)" opacity="0.13"`) +
  shape(`filter="url(#${p}GrainLight)" opacity="0.15"`);

export const luminance = (rgb) => (rgb[0] + rgb[1] + rgb[2]) / 3;

export function litScale(lum, local, reference) {
  const k = Math.round(100 * lum(local) / lum(reference));
  return (color) => `color-mix(in srgb, ${color} ${k}%, #000)`;
}

export function grainPatch(p, id, x, y, w, h, rx) {
  const rect = (extra) => `<rect x="${x}" y="${y}" width="${w}" height="${h}" ${extra}/>`;
  return `<clipPath id="${id}"><rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${rx}"/></clipPath>
    <g clip-path="url(#${id})">${grain(p, rect)}</g>`;
}
