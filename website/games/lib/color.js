export const rgbOf = (hex) => [
  parseInt(hex.slice(1, 3), 16),
  parseInt(hex.slice(3, 5), 16),
  parseInt(hex.slice(5, 7), 16),
];

export const mixRgb = (a, b, t) => a.map((v, i) => Math.round(v + (b[i] - v) * t));

export const css = (rgb) => `rgb(${rgb[0]},${rgb[1]},${rgb[2]})`;
