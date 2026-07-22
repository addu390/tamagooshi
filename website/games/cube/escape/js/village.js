import { hash2 } from "../../../lib/math3d.js";

const SEG = 5.2;
const MARGIN = 0.6;

export const SIDEWALK_W = 1.35;
export const MAX_SETBACK = 0.8;
export const KERB_H = 0.13;
export const STORY_H = 1.35;
export const SLOTS = 3;

const BRICK_STYLES = [
  { main: "wall_brick", corner: ["wall_brick_l", "wall_brick_r"], grey: true },
  { main: "wall_sand", corner: ["wall_sand_l", "wall_sand_r"], grey: false },
  { main: "wall_stone", corner: ["wall_brick_l", "wall_brick_r"], grey: true },
];
const TIMBER = ["timber_horiz", "wall_timber"];

const KINDS = ["brick", "brick", "timber", "timber", "cottage", "terrace"];

export function slotZ(f, i) {
  return f.z0 + MARGIN + ((f.z1 - f.z0 - MARGIN * 2) * (i + 0.5)) / SLOTS;
}

export function* facades(rowMin, rowMax, halfTrack) {
  for (const side of [-1, 1]) {
    const kMin = Math.floor(rowMin / SEG) - 1;
    const kMax = Math.floor(rowMax / SEG) + 1;
    for (let k = kMin; k <= kMax; k++) {
      const rAt = (kk, n) => hash2(kk * 4 + (side > 0 ? 2 : 0), n);
      const setbackAt = (kk) => [0, 0.4, MAX_SETBACK][Math.floor(rAt(kk, 45.7) * 3)];
      const r = (n) => rAt(k, n);
      if (r(47.9) < 0.1) continue;
      const style = BRICK_STYLES[Math.floor(r(3.7) * BRICK_STYLES.length)];
      const kind = KINDS[Math.floor(r(51.3) * KINDS.length)];
      const gap = r(41.3) < 0.45 ? 0.35 + r(43.1) * 0.85 : 0.12;
      const stories = kind === "brick" && r(11.4) < 0.28 ? 1 : 2;
      const setback = setbackAt(k);
      const prevExists = rAt(k - 1, 47.9) >= 0.1;
      const sideRoom = prevExists && setbackAt(k - 1) - setback >= 0.55;
      yield {
        side,
        kind,
        z0: k * SEG + gap,
        z1: (k + 1) * SEG - 0.12,
        xWall: side * (halfTrack + SIDEWALK_W + setback),
        stories,
        ground: style.main,
        corner: style.corner,
        upper: TIMBER[Math.floor(r(6.2) * TIMBER.length)],
        roof: kind === "cottage" ? "thatch" : (r(9.4) > 0.5 ? "roof_red" : "roof_grey"),
        doorSlot: r(17.3) < 0.75 ? Math.floor(r(19.1) * SLOTS) : -1,
        doorLit: r(23.7) > 0.5,
        doorMetal: style.grey && r(71.9) < 0.2,
        lit: [r(31.1) > 0.5, r(33.5) > 0.5, r(37.9) > 0.5],
        windowParity: Math.floor(r(57.1) * 2),
        crates: r(61.7) > 0.5
          ? {
              at: r(63.1),
              stack: Math.floor(r(67.3) * 3),
              beside: sideRoom && r(69.7) < 0.55,
              off: r(77.3) < 0.3 ? 0.3 + r(79.9) * 0.25 : 0,
              kinds: [Math.floor(r(81.1) * 3), Math.floor(r(83.3) * 3),
                      Math.floor(r(87.7) * 3)],
            }
          : null,
      };
    }
  }
}
