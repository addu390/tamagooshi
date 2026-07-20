import { rad, project } from "./core.js";
import { selectTab } from "../components/tabs.js";
import { INVADER } from "../components/sprite.js";

const PRESET = window.TAMA_PRESET || {};
const T = (key, fallback) => (PRESET.text || {})[key] || fallback;

const W = 198, D = 104, H = 44;
const hx = W / 2, hy = D / 2, hz = H / 2;
const DEFAULT_YAW = -34, DEFAULT_PITCH = 30;
const YAW_MIN = -52, YAW_MAX = -16, PITCH_MIN = 20, PITCH_MAX = 40;
const GD = 58;
let VB = { x: -258, y: -212, w: 520, h: 414 };

const C = Object.assign({
  sil: "#1b1d23", det: "#b9bec8", grid: "#e9ebf0",
  dimline: "#c7ccd4", dimtext: "#9298a4", call: "#aeb4be",
  shell: "#ced3db", shade0: [214, 217, 224], bezel: "#d8dce3",
  well: "#f4f6f9", btnPocket: "#c3c9d2", btnPocketDeep: "#b4bbc5",
  btnCap: "#d4d9e0", btnCapHi: "#e4e8ee",
  hat: "#3f444c", hatHole: "#14161a", hatSlot: "#2b2f36",
  hatGlass: "#14161c", hatGlassLine: "#4a5160", vent: "#565b64",
}, PRESET.deviceColors || {});

const SIL = C.sil, DET = C.det, GRID = C.grid;
const DIMLINE = C.dimline, DIMTEXT = C.dimtext, CALL = C.call;
const LIGHT = [-0.35, -0.55, 0.78];

const V = [
  [-hx, -hy, -hz], [hx, -hy, -hz], [hx, hy, -hz], [-hx, hy, -hz],
  [-hx, -hy, hz], [hx, -hy, hz], [hx, hy, hz], [-hx, hy, hz],
];
const FACES = [
  { id: "top", idx: [4, 5, 6, 7], o: 4, u: [1, 0, 0], v: [0, 1, 0], w: W, h: D },
  { id: "bottom", idx: [0, 3, 2, 1], o: 0, u: [1, 0, 0], v: [0, 1, 0], w: W, h: D },
  { id: "east", idx: [1, 2, 6, 5], o: 1, u: [0, 1, 0], v: [0, 0, 1], w: D, h: H },
  { id: "west", idx: [0, 4, 7, 3], o: 0, u: [0, 1, 0], v: [0, 0, 1], w: D, h: H },
  { id: "south", idx: [3, 7, 6, 2], o: 3, u: [1, 0, 0], v: [0, 0, 1], w: W, h: H },
  { id: "north", idx: [0, 1, 5, 4], o: 0, u: [1, 0, 0], v: [0, 0, 1], w: W, h: H },
];

const CORNER_R = 8;
const DIMS = [
  { a: 3, b: 2, out: [0, GD * 0.85, 0], label: "48 mm" },
  { a: 6, b: 5, out: [GD * 0.95, 0, 0], label: "24 mm" },
  { a: 3, b: 7, out: [-GD * 0.9, 0, 0], label: "13.5 mm" },
];

const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const add = (a, b) => [a[0] + b[0], a[1] + b[1], a[2] + b[2]];
const mul = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const norm = (a) => { const l = Math.hypot(a[0], a[1], a[2]) || 1; return [a[0] / l, a[1] / l, a[2] / l]; };
const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

function faceNormal(f) {
  const a = V[f.idx[0]], b = V[f.idx[1]], d = V[f.idx[3]];
  return norm(cross(sub(b, a), sub(d, a)));
}

function shade(nWorld, A, E) {
  const p = project(nWorld, A, E), o = project([0, 0, 0], A, E);
  const nv = norm([p.x - o.x, p.y - o.y, p.depth - o.depth]);
  const t = Math.max(0.6, Math.min(1, 0.63 + 0.4 * dot(nv, LIGHT)));
  const c0 = C.shade0, c1 = [255, 255, 255];
  const c = c0.map((v0, i) => Math.round(v0 + (c1[i] - v0) * ((t - 0.6) / 0.4)));
  return `rgb(${c[0]},${c[1]},${c[2]})`;
}

function hull2d(pts) {
  const p = pts.slice().sort((a, b) => a.x - b.x || a.y - b.y);
  const cr = (o, a, b) => (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
  const chain = (list) => {
    const out = [];
    for (const q of list) {
      while (out.length > 1 && cr(out[out.length - 2], out[out.length - 1], q) <= 0) out.pop();
      out.push(q);
    }
    return out.slice(0, -1);
  };
  return chain(p).concat(chain(p.slice().reverse()));
}

function roundedPoly(pts, r) {
  const n = pts.length;
  let d = "";
  for (let i = 0; i < n; i++) {
    const p0 = pts[(i + n - 1) % n], p1 = pts[i], p2 = pts[(i + 1) % n];
    const l1 = Math.hypot(p1.x - p0.x, p1.y - p0.y) || 1;
    const l2 = Math.hypot(p2.x - p1.x, p2.y - p1.y) || 1;
    const c1 = Math.min(r, l1 / 2), c2 = Math.min(r, l2 / 2);
    const ax = p1.x - (p1.x - p0.x) / l1 * c1, ay = p1.y - (p1.y - p0.y) / l1 * c1;
    const bx = p1.x + (p2.x - p1.x) / l2 * c2, by = p1.y + (p2.y - p1.y) / l2 * c2;
    d += `${i ? "L" : "M"}${ax.toFixed(1)} ${ay.toFixed(1)} Q ${p1.x.toFixed(1)} ${p1.y.toFixed(1)} ${bx.toFixed(1)} ${by.toFixed(1)} `;
  }
  return d + "Z";
}

const DIP_H = 2.8, DIP_START = 0.80, DIP_RAMP = 0.82;
function carveDip(pts, A, E) {
  const corner = project(V[2], A, E), start = project(V[3], A, E);
  const eq = (p, q) => Math.abs(p.x - q.x) < 0.01 && Math.abs(p.y - q.y) < 0.01;
  const i = pts.findIndex((p) => eq(p, corner));
  if (i < 0) return pts;
  const along = (f, lift) => project([-hx + f * W, hy, -hz + lift], A, E);
  const seg = [along(DIP_START, 0), along(DIP_RAMP, DIP_H), along(1, DIP_H)];
  const n = pts.length;
  if (eq(pts[(i + n - 1) % n], start)) return pts.slice(0, i).concat(seg, pts.slice(i + 1));
  if (eq(pts[(i + 1) % n], start)) return pts.slice(0, i).concat(seg.reverse(), pts.slice(i + 1));
  return pts;
}

function southPress(A, E, k) {
  if (!k) return { x: 0, y: 0 };
  const o = project([0, 0, 0], A, E);
  const n = project([0, -k, 0], A, E);
  const u = project([1, 0, 0], A, E), v = project([0, 0, 1], A, E);
  const ux = u.x - o.x, uy = u.y - o.y, vx = v.x - o.x, vy = v.y - o.y;
  const nx = n.x - o.x, ny = n.y - o.y;
  const det = ux * vy - uy * vx || 1e-6;
  return { x: (nx * vy - ny * vx) / det, y: (ux * ny - uy * nx) / det };
}

function faceFrame(originIdx, uUnit, vUnit, A, E) {
  const O = V[originIdx], o = project(O, A, E);
  const a = project(add(O, uUnit), A, E), b = project(add(O, vUnit), A, E);
  return `matrix(${(a.x - o.x).toFixed(4)},${(a.y - o.y).toFixed(4)},${(b.x - o.x).toFixed(4)},${(b.y - o.y).toFixed(4)},${o.x.toFixed(3)},${o.y.toFixed(3)})`;
}

const ACC = "var(--accent)", ACC_DK = "var(--accent-strong)";
const SCR_W = (26.4 / 48) * W, SCR_H = (14.9 / 24) * D;
const SCREEN = { x: 0.09 * W, y: (D - SCR_H) / 2, w: SCR_W, h: SCR_H };
const BTN_W = (3.2 / 48) * W, BTN_H = (12.8 / 24) * D;
const BTN = { x: 0.80 * W - BTN_W / 2, y: (D - BTN_H) / 2, w: BTN_W, h: BTN_H };

const STRIPS = ["#e0655d", "#e39a45", "#e6cf55", "#57b268", "#4aa0cf", "#5566c8", "#a15fc4"];

function miniCells(art, ox, oy, ps, fill) {
  let cells = "";
  for (let r = 0; r < art.length; r++) for (let c = 0; c < art[r].length; c++) if (art[r][c] === "#")
    cells += `<rect x="${(ox + c * ps).toFixed(2)}" y="${(oy + r * ps).toFixed(2)}" width="${(ps + 0.4).toFixed(2)}" height="${(ps + 0.4).toFixed(2)}" fill="${fill}"/>`;
  return cells;
}

function pixelCells(art, fill, factor) {
  const n = art[0].length, m = art.length;
  const ps = Math.min(SCREEN.w / n, SCREEN.h / m) * (factor || 0.82);
  return miniCells(art, SCREEN.x + (SCREEN.w - n * ps) / 2, SCREEN.y + (SCREEN.h - m * ps) / 2, ps, fill);
}

const SCREEN_SOFT = "#8a909c";
const DEV_NAME = PRESET.deviceName || "GOOSHI";

function screenChrome(name) {
  const x = SCREEN.x, y = SCREEN.y, w = SCREEN.w, h = SCREEN.h;
  return `${scrText(x + 7, y + 11, "12:47", 6.5, SCREEN_SOFT, 700)}
    ${scrText(x + w - 7, y + 11, "&#9679; LINK", 6, SCREEN_SOFT, 700, "end")}
    <line x1="${(x + 6).toFixed(1)}" y1="${(y + 14.5).toFixed(1)}" x2="${(x + w - 6).toFixed(1)}" y2="${(y + 14.5).toFixed(1)}" stroke="${SCREEN_SOFT}" stroke-width="0.5" opacity="0.5" vector-effect="non-scaling-stroke"/>
    ${scrText(x + 7, y + h - 6, name, 6.5, SCREEN_SOFT, 700)}`;
}

function stripCells() {
  const sx = SCREEN.x, sy = SCREEN.y, sw = SCREEN.w, sh = SCREEN.h, bw = sw / STRIPS.length;
  return STRIPS.map((c, i) => `<rect x="${(sx + i * bw).toFixed(2)}" y="${sy.toFixed(2)}" width="${(bw + 0.5).toFixed(2)}" height="${sh.toFixed(2)}" fill="${c}"/>`).join("");
}

function screenArt(opts) {
  if (opts && opts.highlight) return "";
  if (opts && opts.group === "controls") return stripCells();
  return screenChrome(DEV_NAME) + `<g transform="translate(0,2.5)">${pixelCells(INVADER, "#3a3f47", 0.58)}</g>`;
}

function zLift(A, E, h) {
  const o = project([0, 0, 0], A, E), pu = project([1, 0, 0], A, E);
  const pv = project([0, 1, 0], A, E), pz = project([0, 0, 1], A, E);
  const a11 = pu.x - o.x, a21 = pu.y - o.y, a12 = pv.x - o.x, a22 = pv.y - o.y;
  const bx = (pz.x - o.x) * h, by = (pz.y - o.y) * h;
  const det = a11 * a22 - a12 * a21 || 1;
  return { x: (bx * a22 - a12 * by) / det, y: (a11 * by - bx * a21) / det };
}

function buttonPill(lift) {
  const cap = lift || { x: 0, y: 0 };
  const pill = (dx, dy, fill, stroke) =>
    `<rect x="${(BTN.x + dx).toFixed(2)}" y="${(BTN.y + dy).toFixed(2)}" width="${BTN.w}" height="${BTN.h}" rx="${BTN.w / 2}" style="fill:${fill}"${stroke ? ` stroke="${SIL}" stroke-width="1.1" vector-effect="non-scaling-stroke"` : ""}/>`;
  let side = pill(0, 0, ACC, true);
  const steps = 10;
  for (let i = 1; i <= steps; i++) side += pill(cap.x * i / steps, cap.y * i / steps, ACC, false);
  return `<g class="btn-a" style="cursor:pointer">${side + pill(cap.x, cap.y, `color-mix(in srgb, ${ACC} 74%, #fff)`, true)}</g>`;
}

const TOP = { o: 4, u: [1, 0, 0], v: [0, 1, 0], feats: (opts) => `
  <rect x="${SCREEN.x - 4}" y="${SCREEN.y - 4}" width="${SCREEN.w + 6}" height="${SCREEN.h + 6}" rx="9.5" fill="${DET}"/>
  <rect x="${SCREEN.x - 1.5}" y="${SCREEN.y - 1.5}" width="${SCREEN.w + 3}" height="${SCREEN.h + 3}" rx="8" fill="${C.bezel}"/>
  <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="#ffffff"/>
  <clipPath id="scrClip"><rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7"/></clipPath>
  <g clip-path="url(#scrClip)">${screenArt(opts)}</g>
  <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="none" stroke="${SIL}" stroke-width="1.2" vector-effect="non-scaling-stroke"/>
  ${buttonPill(opts && opts.lift)}
  ` };

const SEAM_Y = 0.45 * H;
const SOUTH = { o: 3, u: [1, 0, 0], v: [0, 0, 1], feats: (opts) => {
  const inset = 0.10 * W, pocket = 10.5, cap = 7.5, cy = H / 2;
  const sl = (opts && opts.sideLift) || { x: 0, y: 0 };
  const ex = inset, bx = W - inset;
  const st = { x: 0.20 * W, w: 0.60 * W, h: 24 };

  const sq = (cx, midY, r, rx, fill, stroke, swd) =>
    `<rect x="${(cx - r).toFixed(1)}" y="${(midY - r).toFixed(1)}" width="${r * 2}" height="${r * 2}" rx="${rx}" fill="${fill}"${stroke ? ` stroke="${stroke}" stroke-width="${swd || 1}" vector-effect="non-scaling-stroke"` : ""}/>`;
  const slotY = cy - st.h / 2;
  const slot = (cx) => `<rect x="${(cx - 10).toFixed(1)}" y="${(slotY - 1.7).toFixed(1)}" width="20" height="3.4" rx="1.7" fill="${C.hatSlot}"/>`;
  const seam = (x1, x2) => x2 - x1 < 4 ? "" : `<line x1="${x1.toFixed(1)}" y1="${cy.toFixed(1)}" x2="${x2.toFixed(1)}" y2="${cy.toFixed(1)}" stroke="${DET}" stroke-width="0.9" vector-effect="non-scaling-stroke"/>`;

  return `
  ${seam(4, ex - pocket - 2)}${seam(ex + pocket + 2, st.x - 2)}${seam(st.x + st.w + 2, bx - pocket - 2)}${seam(bx + pocket + 2, W - 4)}
  ${sq(ex, cy, pocket, 3.5, "none", DET, 0.9)}
  <rect x="${st.x.toFixed(1)}" y="${(cy - st.h / 2).toFixed(1)}" width="${st.w.toFixed(1)}" height="${st.h}" rx="2.5" fill="${C.well}" stroke="${DET}" stroke-width="1" stroke-dasharray="4 3" vector-effect="non-scaling-stroke"/>
  <g transform="translate(${(st.x + st.w / 2).toFixed(1)},${cy.toFixed(1)}) scale(1,-1)"><text text-anchor="middle" dominant-baseline="central" font-family="'JetBrains Mono',ui-monospace,monospace" font-size="8.5" font-weight="700" letter-spacing="0.08em" fill="${DIMTEXT}">${T("logoText", "YOUR LOGO")}</text></g>
  ${slot(st.x + st.w / 2 - 33)}${slot(st.x + st.w / 2 + 33)}
  <g class="btn-side" style="cursor:pointer">
  ${sq(bx, cy, pocket, 3.5, C.btnPocket, SIL, 1)}
  ${sq(bx, cy, pocket - 1.6, 3, C.btnPocketDeep, null)}
  <g transform="translate(${sl.x.toFixed(2)},${sl.y.toFixed(2)})">
  ${sq(bx, cy, cap, 2.5, C.btnCap, SIL, 1.1)}
  <line x1="${(bx - cap + 2).toFixed(1)}" y1="${(cy - cap + 1.6).toFixed(1)}" x2="${(bx + cap - 2).toFixed(1)}" y2="${(cy - cap + 1.6).toFixed(1)}" stroke="${C.btnCapHi}" stroke-width="1.4" stroke-linecap="round" vector-effect="non-scaling-stroke"/>
  </g>
  </g>`;
} };

const WEST = { o: 0, u: [0, 1, 0], v: [0, 0, 1], feats: (opts) => {
  const hx0 = 10, hw = D - 20, hy0 = 7, hh = 0.50 * H;
  const cols = 9, rows = 2, gx = 3.4;
  const cw = (hw - gx * (cols + 1)) / cols;
  const ch = (hh - gx * (rows + 1)) / rows;

  let holes = "";
  for (let r = 0; r < rows; r++) for (let c = 0; c < cols; c++)
    holes += `<rect x="${(hx0 + gx + c * (cw + gx)).toFixed(1)}" y="${(hy0 + gx + r * (ch + gx)).toFixed(1)}" width="${cw.toFixed(1)}" height="${ch.toFixed(1)}" rx="0.8" fill="${C.hatHole}"/>`;

  const vy = 0.78 * H;
  const colX = (c) => hx0 + gx + c * (cw + gx);
  const slotW = 2 * cw + gx, slotY = hy0 + hh, slotH = vy + ch * 0.4 - slotY;
  const slot = (c) => `<rect x="${colX(c).toFixed(1)}" y="${slotY.toFixed(1)}" width="${slotW.toFixed(1)}" height="${slotH.toFixed(1)}" rx="0.8" fill="${C.hatSlot}"/>`;

  const ix = colX(0);
  const glow = opts && opts.ir
    ? `<rect x="${ix.toFixed(1)}" y="${slotY.toFixed(1)}" width="${slotW.toFixed(1)}" height="${slotH.toFixed(1)}" fill="#ff5148" opacity="0">
        <animate attributeName="opacity" values="0;0;0.4;0.12;0.45;0;0" keyTimes="0;0.5;0.62;0.7;0.78;0.92;1" dur="3.4s" repeatCount="indefinite"/>
      </rect>`
    : "";
  const glass = `
  <rect x="${ix.toFixed(1)}" y="${slotY.toFixed(1)}" width="${slotW.toFixed(1)}" height="${slotH.toFixed(1)}" rx="0.8" fill="${C.hatGlass}" stroke="${C.hatGlassLine}" stroke-width="0.6" vector-effect="non-scaling-stroke"/>
  <clipPath id="irClip"><rect x="${ix.toFixed(1)}" y="${slotY.toFixed(1)}" width="${slotW.toFixed(1)}" height="${slotH.toFixed(1)}" rx="0.8"/></clipPath>
  <g clip-path="url(#irClip)">
    <polygon points="${(ix + slotW * 0.16).toFixed(1)},${slotY.toFixed(1)} ${(ix + slotW * 0.38).toFixed(1)},${slotY.toFixed(1)} ${(ix + slotW * 0.24).toFixed(1)},${(slotY + slotH).toFixed(1)} ${(ix + slotW * 0.02).toFixed(1)},${(slotY + slotH).toFixed(1)}" fill="#ffffff" opacity="0.16"/>
    <polygon points="${(ix + slotW * 0.52).toFixed(1)},${slotY.toFixed(1)} ${(ix + slotW * 0.60).toFixed(1)},${slotY.toFixed(1)} ${(ix + slotW * 0.46).toFixed(1)},${(slotY + slotH).toFixed(1)} ${(ix + slotW * 0.38).toFixed(1)},${(slotY + slotH).toFixed(1)}" fill="#ffffff" opacity="0.07"/>
    ${glow}
  </g>`;
  const vents = [-1, 0, 1].map((k) => `<circle cx="${(0.50 * D + k * 7).toFixed(1)}" cy="${vy.toFixed(1)}" r="1.7" fill="${C.vent}"/>`).join("");

  return `
  <rect x="${hx0}" y="${hy0.toFixed(1)}" width="${hw}" height="${hh.toFixed(1)}" rx="3" fill="${C.hat}" stroke="${SIL}" stroke-width="1" vector-effect="non-scaling-stroke"/>
  ${holes}
  ${slot(7)}${glass}
  ${vents}`;
} };

const FEATURED = { top: TOP, south: SOUTH, west: WEST };

const anchorOf = (face, ax, ay) =>
  face === "top" ? [-hx + ax, -hy + ay, hz]
    : face === "south" ? [-hx + ax, hy, -hz + ay]
      : face === "north" ? [-hx + ax, -hy, -hz + ay]
        : [hx, -hy + ax, -hz + ay];

const CALLOUTS = [
  { g: "controls", face: "top", p: anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), rise: 84, run: 0, label: "COLOR LCD", sub: "135 × 240" },
  { g: "controls", face: "top", p: anchorOf("top", BTN.x + BTN.w / 2, BTN.y + BTN.h / 2), rise: 96, run: 58, label: "BUTTON A", sub: "" },
  { g: "controls", face: "south", p: anchorOf("south", 0.9 * W, 0.5 * H), rise: -56, run: 14, label: "POWER", sub: "6s = off" },
  { g: "controls", p: [-hx + 0.28 * W, -hy, hz], rise: 60, run: -40, label: "BUTTON B", sub: "rear", show: true, dashed: true },
  { g: "io", face: "top", p: anchorOf("top", 0, 0.16 * D), rise: 46, run: -54, label: "3D ANTENNA", sub: "" },
  { g: "io", face: "top", p: anchorOf("top", 0, 0.46 * D), rise: 14, run: -70, label: "LED", sub: "" },
  { g: "io", face: "top", p: anchorOf("top", 0, 0.76 * D), rise: -18, run: -80, label: "IR TX", sub: "" },
  { g: "io", face: "top", p: anchorOf("top", 0.575 * W, 0.60 * D), rise: 92, run: 34, label: "MIC", sub: "" },
  { g: "io", p: [hx, hy, 0], rise: -24, run: 66, label: "USB-C", sub: "", show: true },
];

const MASCOT_ART = PRESET.mascotArt
  || ["#.......#", "##.....##", ".#######.", "#########", "#.#...#.#", "#########", "#..###..#", ".#.###.#."];

function scrText(x, y, s, size, fill, weight, anchor) {
  return `<text x="${x.toFixed(1)}" y="${y.toFixed(1)}" text-anchor="${anchor || "start"}" font-family="'JetBrains Mono',ui-monospace,monospace" font-size="${size}" font-weight="${weight || 700}" style="fill:${fill}">${s}</text>`;
}

function metricScreen() {
  const x = SCREEN.x, y = SCREEN.y, w = SCREEN.w, h = SCREEN.h, cx = x + w / 2;
  const spark = [0.15, 0.3, 0.24, 0.46, 0.4, 0.58, 0.54, 0.78].map((v, i) => `${(x + 10 + i * (w - 20) / 7).toFixed(1)},${(y + h - 8 - v * (h * 0.2)).toFixed(1)}`).join(" ");
  return `${scrText(x + 9, y + 15, "MRR", 8, "#8a909c")}
    ${scrText(x + w - 9, y + 15, "&#9650; 8.2%", 8, ACC_DK, 700, "end")}
    ${scrText(cx, y + h * 0.5, "$12.4k", 19, "#1b1d23", 800, "middle")}
    <polyline points="${spark}" fill="none" style="stroke:${ACC}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round"/>`;
}

const MASCOT_SAD = PRESET.mascotSadArt
  || ["#.......#", "##.....##", ".#######.", "#########", "#.#...#.#", "#########", ".#.###.#.", "#..###..#"];

function alertScreen() {
  const x = SCREEN.x, y = SCREEN.y, w = SCREEN.w, h = SCREEN.h, bh = h * 0.3;
  const ps = (h - bh - 16) / MASCOT_SAD.length;
  return `<rect x="${x}" y="${y}" width="${w}" height="${bh.toFixed(1)}" fill="${DENY}"/>
    ${scrText(x + 8, y + bh / 2 + 2.5, "&#9650; ALERT", 7.5, "#ffffff", 800)}
    ${scrText(x + w - 8, y + bh / 2 + 2.5, "×3", 7.5, "#ffffff", 800, "end")}
    ${miniCells(MASCOT_SAD, x + 12, y + bh + 8, ps, SCREEN_INK)}
    ${scrText(x + w * 0.63, y + h * 0.62, "CI FAILING", 10.5, SCREEN_INK, 800, "middle")}
    ${scrText(x + w * 0.63, y + h * 0.82, "main · 12m", 6.5, SCREEN_SOFT, 700, "middle")}`;
}

const SCREEN_BG = "#ffffff", SCREEN_INK = "#1b1d23";
const mascotScreen = (ink) => screenChrome(DEV_NAME) + `<g transform="translate(0,2.5)">${pixelCells(MASCOT_ART, ink, 0.56)}</g>`;
const brandCalls = (label, sub) => (A, E) => callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), 84, -22, label, sub, A, E)
  + callout(anchorOf("top", SCREEN.x + 4, SCREEN.y + SCREEN.h - 4), -34, -70, "DEV NAME", "yours", A, E);

const BRAND_SCREENS = [
  {
    caption: T("mascotCaption", "Your mascot, on its home screen"), bg: SCREEN_BG, screen: () => mascotScreen(SCREEN_INK),
    calls: brandCalls(T("mascotLabel", "YOUR MASCOT"), T("mascotSub", "any sprite pack")),
  },
  {
    caption: T("themeCaption", "Themes to match: light or dark"), bg: SCREEN_INK, screen: () => mascotScreen(SCREEN_BG),
    calls: brandCalls(T("themeLabel", "DARK THEME"), T("themeSub", "one of many")),
  },
  {
    caption: "Your pulse, live metrics",
    bg: SCREEN_BG,
    screen: metricScreen,
    calls: (A, E) => callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), 84, -18, "CUSTOM PULSE", "MRR · stars · live", A, E),
  },
  {
    caption: "System & brand alerts",
    bg: SCREEN_BG,
    screen: () => alertScreen(),
    calls: (A, E) => callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), 84, -18, "SYSTEM ALERTS", "brand-styled", A, E),
  },
];

function overlayBrand(A, E, idx) {
  const s = BRAND_SCREENS[((idx % BRAND_SCREENS.length) + BRAND_SCREENS.length) % BRAND_SCREENS.length];
  const fr = faceFrame(TOP.o, TOP.u, TOP.v, A, E);
  const g = `<g transform="${fr}">
    <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="${s.bg}"/>
    <clipPath id="brandClip"><rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7"/></clipPath>
    <g clip-path="url(#brandClip)">${s.screen()}</g>
    <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="none" stroke="${SIL}" stroke-width="1.4" vector-effect="non-scaling-stroke"/>
  </g>`;
  return g + s.calls(A, E);
}

function shooterArt(frame) {
  const cols = 11, rows = 11;
  const g = Array.from({ length: rows }, () => Array(cols).fill("."));
  const sh = frame % 2;
  for (let c = 1; c < cols - 1; c++) {
    if ((c + sh) % 2 === 0) g[2][c] = "#";
    if ((c + sh) % 2 === 1) g[3][c] = "#";
    if ((c + sh) % 2 === 0) g[4][c] = "#";
  }
  g[8][5] = "#";
  g[9][4] = g[9][5] = g[9][6] = "#";
  g[10][3] = g[10][4] = g[10][5] = g[10][6] = g[10][7] = "#";
  const by = 7 - (frame % 3) * 2;
  g[by][5] = "#";
  g[by - 1][5] = "#";
  return g.map((r) => r.join(""));
}

function overlayGames(A, E, idx) {
  const fr = faceFrame(TOP.o, TOP.u, TOP.v, A, E);
  const score = scrText(SCREEN.x + 7, SCREEN.y + 10, `SCORE ${String(420 + idx * 20).padStart(4, "0")}`, 6, SCREEN_SOFT, 700)
    + scrText(SCREEN.x + SCREEN.w - 7, SCREEN.y + 10, "HI 0860", 6, SCREEN_SOFT, 700, "end");
  const g = `<g transform="${fr}">
    <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="${SCREEN_BG}"/>
    <clipPath id="gameClip"><rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7"/></clipPath>
    <g clip-path="url(#gameClip)">${score}${pixelCells(shooterArt(idx), SCREEN_INK, 0.92)}</g>
    <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="none" stroke="${SIL}" stroke-width="1.4" vector-effect="non-scaling-stroke"/>
  </g>`;
  const pilot = callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), 84, -22, T("gameMascotLabel", "MASCOT IN GAME"), T("gameMascotSub", "your pixel ship"), A, E);
  const ctrl = callout(anchorOf("top", BTN.x + BTN.w / 2, BTN.y + BTN.h / 2), 72, 46, "PLAY IN BROWSER", "or on device", A, E);
  const more = callout(anchorOf("top", SCREEN.x + 4, SCREEN.y + SCREEN.h - 4), -34, -44, "GAME LIBRARY", "per brand", A, E);
  return g + pilot + ctrl + more;
}

const DENY = "#d16b6b";

const ROBOT_HAPPY = [
  "...#...#...",
  "...#...#...",
  ".#########.",
  "#.........#",
  "#.##...##.#",
  "#.##...##.#",
  "#.........#",
  "#.#.....#.#",
  "#..#####..#",
  ".#########.",
  "..#.....#..",
];

function approveDeny() {
  const cx = SCREEN.x + SCREEN.w / 2, y = SCREEN.y + SCREEN.h * 0.48;
  return scrText(cx, SCREEN.y + 17, "APPROVE?", 9.5, SIL, 800, "middle")
    + `<path d="M ${cx - 26} ${y} l 5 6 l 11 -13" fill="none" style="stroke:${ACC}" stroke-width="2.8" stroke-linecap="round" stroke-linejoin="round"/>`
    + `<path d="M ${cx + 14} ${y - 6} l 12 12 M ${cx + 26} ${y - 6} l -12 12" fill="none" stroke="${DENY}" stroke-width="2.8" stroke-linecap="round"/>`;
}

const MIC = [
  "....###....",
  "...#####...",
  "...#####...",
  "...#####...",
  ".#..###..#.",
  ".#..###..#.",
  ".#.......#.",
  "..#######..",
  ".....#.....",
  ".....#.....",
  "...#####...",
];

const AGENTS = { p: [hx, -hy + 0.9 * D, -hz], rise: -62, run: -16, label: PRESET.agentsLabel || "CLAUDE · CURSOR" };
const agentsCall = (A, E) => callout(AGENTS.p, AGENTS.rise, AGENTS.run, AGENTS.label, PRESET.agentsSub || "your agents", A, E);
const coworkCalls = (A, E) => callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), 84, -22, T("coworkLabel", "CO-WORK WITH AI"), "reacts live", A, E)
  + agentsCall(A, E);

function workingDots() {
  const cx = SCREEN.x + SCREEN.w / 2, y = SCREEN.y + SCREEN.h - 7;
  return [-1, 0, 1].map((k, i) =>
    `<circle cx="${(cx + k * 8).toFixed(1)}" cy="${y.toFixed(1)}" r="1.8" style="fill:${ACC}">
      <animate attributeName="opacity" values="0.15;1;0.15" dur="1.2s" begin="${(i * 0.25).toFixed(2)}s" repeatCount="indefinite"/>
    </circle>`).join("");
}

const AI_MASCOT = PRESET.aiMascotArt || ROBOT_HAPPY;
const AI_SCREENS = [
  {
    caption: T("coworkCaption", "Co-work with AI: it reacts live"),
    screen: () => `<g transform="translate(0,-3)">${pixelCells(AI_MASCOT, SCREEN_INK, 0.66)}</g>` + workingDots(),
    calls: coworkCalls,
  },
  {
    caption: "Hold to talk: voice prompts straight to your agent",
    screen: () => pixelCells(MIC, SCREEN_INK, 0.74),
    calls: (A, E) => callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h / 2), 84, -22, "HOLD TO TALK", "voice prompts", A, E)
      + agentsCall(A, E),
  },
  {
    caption: "You approve or deny every action",
    screen: () => approveDeny(),
    calls: (A, E) => callout(anchorOf("top", SCREEN.x + SCREEN.w / 2, SCREEN.y + SCREEN.h * 0.48), 84, -22, "APPROVE OR DENY", "you decide", A, E)
      + agentsCall(A, E),
  },
];

function overlayAI(A, E, idx) {
  const s = AI_SCREENS[((idx % AI_SCREENS.length) + AI_SCREENS.length) % AI_SCREENS.length];
  const fr = faceFrame(TOP.o, TOP.u, TOP.v, A, E);
  const g = `<g transform="${fr}">
    <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="${SCREEN_BG}"/>
    <clipPath id="aiClip"><rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7"/></clipPath>
    <g clip-path="url(#aiClip)">${s.screen()}</g>
    <rect x="${SCREEN.x}" y="${SCREEN.y}" width="${SCREEN.w}" height="${SCREEN.h}" rx="7" fill="none" stroke="${SIL}" stroke-width="1.4" vector-effect="non-scaling-stroke"/>
  </g>`;
  return g + s.calls(A, E);
}

function gridPolys(A, E) {
  const G = 42, N = 7, out = [];
  for (let i = -N; i <= N; i++) {
    for (let j = -N; j <= N; j++) {
      const cx = i * G + G / 2, cy = j * G + G / 2;
      if (Math.abs(cx) < hx + 30 && Math.abs(cy) < hy + 30) continue;
      const pts = [[i * G + 6, j * G + 6], [(i + 1) * G - 6, j * G + 6], [(i + 1) * G - 6, (j + 1) * G - 6], [i * G + 6, (j + 1) * G - 6]]
        .map((q) => project([q[0], q[1], -hz], A, E));
      if (pts.some((p) => Math.abs(p.x) > VB.w / 2 + 20 || Math.abs(p.y) > VB.h / 2 + 20)) continue;
      out.push(pts.map((p) => `${p.x.toFixed(1)},${p.y.toFixed(1)}`).join(" "));
    }
  }
  return out;
}

function arrow(px, py, dx, dy) {
  const l = Math.hypot(dx, dy) || 1; const ux = dx / l, uy = dy / l;
  const s = 5, wdt = 2.2;
  const bx = px + ux * s, by = py + uy * s;
  return `${px.toFixed(1)},${py.toFixed(1)} ${(bx - uy * wdt).toFixed(1)},${(by + ux * wdt).toFixed(1)} ${(bx + uy * wdt).toFixed(1)},${(by - ux * wdt).toFixed(1)}`;
}

function extLine(v, ov) {
  const dx = ov.x - v.x, dy = ov.y - v.y, l = Math.hypot(dx, dy) || 1, ux = dx / l, uy = dy / l;
  const gap = 6, over = 4;
  return `<line x1="${(v.x + ux * gap).toFixed(1)}" y1="${(v.y + uy * gap).toFixed(1)}" x2="${(ov.x + ux * over).toFixed(1)}" y2="${(ov.y + uy * over).toFixed(1)}" stroke="${DIMLINE}" stroke-width="1"/>`;
}

function dim(a3, b3, out3, label, A, E) {
  const ao = add(a3, out3), bo = add(b3, out3);
  const a = project(a3, A, E), b = project(b3, A, E), pao = project(ao, A, E), pbo = project(bo, A, E);
  const mx = (pao.x + pbo.x) / 2, my = (pao.y + pbo.y) / 2;
  const dx = pbo.x - pao.x, dy = pbo.y - pao.y;
  let ax = (pao.x - a.x) + (pbo.x - b.x), ay = (pao.y - a.y) + (pbo.y - b.y);
  const al = Math.hypot(ax, ay) || 1; ax /= al; ay /= al; const off = 13;
  let ang = Math.atan2(dy, dx) * 180 / Math.PI;
  if (ang > 90) ang -= 180; else if (ang < -90) ang += 180;
  const tx = (mx + ax * off).toFixed(1), ty = (my + ay * off).toFixed(1);
  return `${extLine(a, pao)}${extLine(b, pbo)}
  <line x1="${pao.x.toFixed(1)}" y1="${pao.y.toFixed(1)}" x2="${pbo.x.toFixed(1)}" y2="${pbo.y.toFixed(1)}" stroke="${DIMLINE}" stroke-width="1" stroke-linecap="round"/>
  <polygon points="${arrow(pao.x, pao.y, dx, dy)}" fill="${DIMLINE}"/>
  <polygon points="${arrow(pbo.x, pbo.y, -dx, -dy)}" fill="${DIMLINE}"/>
  <text transform="translate(${tx},${ty}) rotate(${ang.toFixed(1)})" text-anchor="middle" dominant-baseline="central" font-family="'JetBrains Mono',ui-monospace,monospace" font-size="11" font-weight="500" letter-spacing="0.02em" fill="${DIMTEXT}">${label}</text>`;
}

function callout(p3, rise, run, label, sub, A, E, dashed, color) {
  const p = project(p3, A, E);
  const x0 = p.x, y0 = p.y, x1 = x0, y1 = y0 - rise, x2 = x1 + run, y2 = y1;
  const end = run >= 0 ? "start" : "end";
  const tx = x2 + (run >= 0 ? 7 : -7);
  const mark = color || SIL, lead = color || CALL, txt = color || "#3f434b", subCol = color || DIMTEXT;
  const lines = sub
    ? `<tspan x="${tx.toFixed(1)}" dy="-0.15em">${label}</tspan><tspan x="${tx.toFixed(1)}" dy="1.25em" font-weight="500" fill="${subCol}">${sub}</tspan>`
    : `<tspan x="${tx.toFixed(1)}">${label}</tspan>`;
  const dash = dashed ? ` stroke-dasharray="3 3"` : "";
  const dot = dashed
    ? `<circle cx="${x0.toFixed(1)}" cy="${y0.toFixed(1)}" r="2.6" fill="none" stroke="${mark}" stroke-width="1.1"/>`
    : `<circle cx="${x0.toFixed(1)}" cy="${y0.toFixed(1)}" r="2.4" fill="${mark}"/>`;
  return `<polyline points="${x0.toFixed(1)},${y0.toFixed(1)} ${x1.toFixed(1)},${y1.toFixed(1)} ${x2.toFixed(1)},${y2.toFixed(1)}" fill="none" stroke="${lead}" stroke-width="1" stroke-linejoin="round"${dash}/>
  ${dot}
  <text x="${tx.toFixed(1)}" y="${y2.toFixed(1)}" text-anchor="${end}" dominant-baseline="central" font-family="'JetBrains Mono',ui-monospace,monospace" font-size="10.5" font-weight="700" letter-spacing="0.04em" fill="${txt}">${lines}</text>`;
}

function render(yawDeg, pitchDeg, opts) {
  opts = opts || {};
  const A = rad(yawDeg), E = rad(pitchDeg);
  const P = V.map((p) => project(p, A, E));
  const originDepth = project([0, 0, 0], A, E).depth;
  const visible = FACES.filter((f) => project(faceNormal(f), A, E).depth - originDepth > 0);
  const topVisible = visible.some((f) => f.id === "top");

  const hullD = roundedPoly(carveDip(hull2d(P), A, E), CORNER_R);
  const shell = `<clipPath id="shellClip"><path d="${hullD}"/></clipPath><path d="${hullD}" fill="${C.shell}"/>`;
  const outline = `<path d="${hullD}" fill="none" stroke="${SIL}" stroke-width="1.2" stroke-linejoin="round"/>`;
  const fills = visible
    .map((f) => ({ f, d: f.idx.reduce((s, i) => s + P[i].depth, 0) / 4 }))
    .sort((m, n) => m.d - n.d)
    .map(({ f }) => `<g transform="${faceFrame(f.o, f.u, f.v, A, E)}"><rect width="${f.w}" height="${f.h}" rx="${CORNER_R}" fill="${shade(faceNormal(f), A, E)}"/></g>`)
    .join("");

  opts.lift = zLift(A, E, 5 * (1 - (opts.press || 0)));
  opts.sideLift = southPress(A, E, 2.4 * (opts.sidePress || 0));
  const feats = visible.map((f) => {
    const spec = FEATURED[f.id];
    if (!spec) return "";
    return `<g transform="${faceFrame(spec.o, spec.u, spec.v, A, E)}">
      <clipPath id="faceClip_${f.id}"><rect width="${f.w}" height="${f.h}" rx="${CORNER_R}"/></clipPath>
      <g clip-path="url(#faceClip_${f.id})">${spec.feats(opts)}</g>
    </g>`;
  }).join("");

  const edgeCount = new Map();
  visible.forEach((f) => {
    for (let i = 0; i < 4; i++) {
      const a = f.idx[i], b = f.idx[(i + 1) % 4];
      const k = a < b ? `${a}_${b}` : `${b}_${a}`;
      edgeCount.set(k, (edgeCount.get(k) || 0) + 1);
    }
  });
  let interior = "";
  edgeCount.forEach((n, k) => {
    if (n < 2) return;
    const [a, b] = k.split("_").map(Number);
    const dx = P[b].x - P[a].x, dy = P[b].y - P[a].y, l = Math.hypot(dx, dy) || 1;
    const t = Math.min(CORNER_R, l / 2 - 1) / l;
    interior += `<line x1="${(P[a].x + dx * t).toFixed(1)}" y1="${(P[a].y + dy * t).toFixed(1)}" x2="${(P[b].x - dx * t).toFixed(1)}" y2="${(P[b].y - dy * t).toFixed(1)}" stroke="${DET}" stroke-width="0.9" stroke-linecap="round"/>`;
  });

  const alpha = opts.annoAlpha == null ? 1 : opts.annoAlpha;
  let overlay = "";
  if (topVisible && opts.highlight === "brand") overlay = overlayBrand(A, E, opts.brandIndex || 0);
  else if (topVisible && opts.highlight === "games") overlay = overlayGames(A, E, opts.gamesIndex || 0);
  else if (topVisible && opts.highlight === "ai") overlay = overlayAI(A, E, opts.aiIndex || 0);
  if (overlay) overlay = `<g opacity="${alpha.toFixed(3)}">${overlay}</g>`;

  const vis = { top: topVisible, south: visible.some((f) => f.id === "south") };
  const dims = opts.dims ? DIMS.map((d) => dim(V[d.a], V[d.b], d.out, d.label, A, E)).join("") : "";
  const calls = opts.group ? CALLOUTS
    .filter((c) => c.g === opts.group && (c.show || vis[c.face]))
    .map((c) => callout(c.p, c.rise, c.run, c.label, c.sub, A, E, c.dashed)).join("") : "";
  const annos = `<g opacity="${alpha.toFixed(3)}">${dims}${calls}</g>`;
  const grid = gridPolys(A, E).map((p) => `<polygon points="${p}"/>`).join("");

  return `<g fill="none" stroke="${GRID}" stroke-width="1">${grid}</g>${shell}<g clip-path="url(#shellClip)">${fills}${feats}</g>${overlay}${interior}${outline}${annos}`;
}

const CW = 7.3;

function accLabel(acc, p3, rise, run, A, E, len) {
  const p = project(p3, A, E);
  acc(p);
  const ex = p.x + run, ey = p.y - rise;
  acc({ x: ex, y: ey });
  if (len) acc({ x: ex + (run >= 0 ? 1 : -1) * (7 + len * CW), y: ey });
}

function accDim(acc, d, A, E) {
  const pao = project(add(V[d.a], d.out), A, E), pbo = project(add(V[d.b], d.out), A, E);
  const mx = (pao.x + pbo.x) / 2, my = (pao.y + pbo.y) / 2;
  const dx = pbo.x - pao.x, dy = pbo.y - pao.y, l = Math.hypot(dx, dy) || 1, w = d.label.length * CW / 2 + 6;
  acc(pao); acc(pbo);
  acc({ x: mx - dy / l * 15 - w, y: my + dx / l * 15 - 8 });
  acc({ x: mx - dy / l * 15 + w, y: my + dx / l * 15 + 8 });
}

function computeViewBox() {
  let minx = Infinity, miny = Infinity, maxx = -Infinity, maxy = -Infinity;
  const acc = (p) => { minx = Math.min(minx, p.x); maxx = Math.max(maxx, p.x); miny = Math.min(miny, p.y); maxy = Math.max(maxy, p.y); };

  const samples = [
    { yaw: DEFAULT_YAW, pitch: DEFAULT_PITCH, dims: true },
    { yaw: -29, pitch: DEFAULT_PITCH, group: "controls" },
    { yaw: -42, pitch: DEFAULT_PITCH, group: "io" },
    { yaw: -30, pitch: 34 },
  ];
  for (const s of samples) {
    const A = rad(s.yaw), E = rad(s.pitch);
    V.forEach((v) => acc(project(v, A, E)));
    if (s.dims) DIMS.forEach((d) => accDim(acc, d, A, E));
    if (s.group) CALLOUTS.filter((c) => c.g === s.group).forEach((c) => accLabel(acc, c.p, c.rise, c.run, A, E));
  }

  const PAD = 30;
  return { x: minx - PAD, y: miny - PAD, w: maxx - minx + 2 * PAD, h: maxy - miny + 2 * PAD };
}
VB = computeViewBox();

export const theme = { SIL, DET, colors: C, mascotArt: MASCOT_ART, roundedPoly };

const MODES = {
  device: { yaw: DEFAULT_YAW, pitch: DEFAULT_PITCH, free: true, caption: "Drag to rotate the ESP32 handheld" },
  brand: { yaw: -30, pitch: 34, highlight: "brand", caption: T("brandModeCaption", "Your brand on every surface: logo, name, theme.") },
  games: { yaw: -30, pitch: 34, highlight: "games", caption: "Play in the browser or on the device." },
  ai: { yaw: -30, pitch: 34, highlight: "ai", caption: PRESET.aiCaption || "Co-work with Claude and Cursor." },
};

const GROUPS = [
  { group: null, dims: true, yaw: DEFAULT_YAW, caption: "48 × 24 × 13.5 mm" },
  { group: "controls", dims: false, yaw: -29, caption: "Color LCD · Button A · Button B · power" },
  { group: "io", dims: false, yaw: -42, caption: "3D antenna · LED · IR · mic · USB-C" },
];

const SLIDE_YAW = { brand: [-30, -34, -26, -38], ai: [-28, -32, -38] };
const slideYaw = (m, i) => { const a = SLIDE_YAW[m]; return a ? a[((i % a.length) + a.length) % a.length] : (MODES[m] ? MODES[m].yaw : DEFAULT_YAW); };

export function initHeroDevice() {
  const mount = document.querySelector(".iso-device");
  if (!mount) return;

  const NS = "http://www.w3.org/2000/svg";
  const svg = document.createElementNS(NS, "svg");
  svg.setAttribute("role", "img");
  svg.setAttribute("aria-label", "Interactive ESP32 handheld diagram");
  mount.appendChild(svg);

  const cap = document.querySelector(".device-cap");
  const dots = document.querySelector(".device-dots");
  const tabs = [].slice.call(document.querySelectorAll(".hero-box .tab"));

  let dotEls = [];
  const buildDots = (n) => {
    if (!dots) return;
    dots.innerHTML = ""; dotEls = [];
    for (let i = 0; i < n; i++) {
      const e = document.createElement("i");
      e.setAttribute("role", "button");
      e.addEventListener("click", () => jumpTo(i));
      dots.appendChild(e); dotEls.push(e);
    }
  };
  const setDot = (i) => dotEls.forEach((e, k) => e.classList.toggle("on", k === i));

  let mode = "device";
  let yaw = DEFAULT_YAW, pitch = DEFAULT_PITCH;
  let brandIndex = 0, gamesIndex = 0, aiIndex = 0, slideTimer = null, anim = null, drag = null;
  let groupIndex = 0, annoAlpha = 1, cycleTimer = null;
  let pressK = 0, pressS = 0, pressAnim = null;

  function pressButton(set) {
    if (pressAnim) cancelAnimationFrame(pressAnim);
    const t0 = performance.now(), dur = 280;
    const step = (now) => {
      const p = Math.min(1, (now - t0) / dur);
      set(Math.sin(p * Math.PI));
      draw();
      if (p < 1) pressAnim = requestAnimationFrame(step);
      else { pressAnim = null; set(0); draw(); }
    };
    pressAnim = requestAnimationFrame(step);
  }

  const opts = () => {
    const base = { annoAlpha, ir: true, press: pressK, sidePress: pressS };
    return mode === "device"
      ? { ...base, dims: GROUPS[groupIndex].dims, group: GROUPS[groupIndex].group }
      : mode === "brand"
        ? { ...base, dims: false, highlight: "brand", brandIndex }
        : mode === "ai"
          ? { ...base, dims: false, highlight: "ai", aiIndex }
          : { ...base, dims: false, highlight: "games", gamesIndex };
  };
  const draw = () => { svg.setAttribute("viewBox", `${VB.x.toFixed(1)} ${VB.y.toFixed(1)} ${VB.w.toFixed(1)} ${VB.h.toFixed(1)}`); svg.innerHTML = render(yaw, pitch, opts()); };
  draw();

  const ease = (k) => (k < 0.5 ? 2 * k * k : 1 - Math.pow(-2 * k + 2, 2) / 2);
  function animateTo(tYaw, tPitch) {
    if (anim) cancelAnimationFrame(anim);
    const s = { yaw, pitch }, t0 = performance.now(), dur = 620;
    const step = (now) => {
      const k = Math.min(1, (now - t0) / dur), e = ease(k);
      yaw = s.yaw + (tYaw - s.yaw) * e;
      pitch = s.pitch + (tPitch - s.pitch) * e;
      draw();
      if (k < 1) anim = requestAnimationFrame(step); else anim = null;
    };
    anim = requestAnimationFrame(step);
  }

  function cycleTo(next) {
    if (anim) cancelAnimationFrame(anim);
    const fromYaw = yaw, toYaw = GROUPS[next].yaw, t0 = performance.now(), dur = 720;
    let swapped = false;
    const step = (now) => {
      const p = Math.min(1, (now - t0) / dur);
      yaw = fromYaw + (toYaw - fromYaw) * ease(p);
      annoAlpha = p < 0.5 ? 1 - p / 0.5 : (p - 0.5) / 0.5;
      if (p >= 0.5 && !swapped) { swapped = true; groupIndex = next; setDot(next); if (cap) cap.textContent = GROUPS[next].caption; }
      draw();
      if (p < 1) anim = requestAnimationFrame(step); else { anim = null; annoAlpha = 1; draw(); }
    };
    anim = requestAnimationFrame(step);
  }

  function startCarousel() {
    if (cycleTimer) clearInterval(cycleTimer);
    cycleTimer = setInterval(() => {
      if (mode === "device" && !drag && !anim) cycleTo((groupIndex + 1) % GROUPS.length);
    }, 3000);
  }
  function stopCarousel() { if (cycleTimer) { clearInterval(cycleTimer); cycleTimer = null; } }

  function slideTo(target) {
    if (anim) cancelAnimationFrame(anim);
    const list = mode === "ai" ? AI_SCREENS : BRAND_SCREENS;
    const cur = mode === "ai" ? aiIndex : brandIndex;
    const next = target == null ? (cur + 1) % list.length : ((target % list.length) + list.length) % list.length;
    const fromYaw = yaw, toYaw = slideYaw(mode, next), t0 = performance.now(), dur = 560;
    let swapped = false;
    const step = (now) => {
      const p = Math.min(1, (now - t0) / dur);
      yaw = fromYaw + (toYaw - fromYaw) * ease(p);
      annoAlpha = p < 0.5 ? 1 - p / 0.5 : (p - 0.5) / 0.5;
      if (p >= 0.5 && !swapped) {
        swapped = true;
        if (mode === "ai") { aiIndex = next; setDot(next); if (cap) cap.textContent = AI_SCREENS[next].caption; }
        else { brandIndex = next; setDot(next); if (cap) cap.textContent = BRAND_SCREENS[next].caption; window.dispatchEvent(new CustomEvent("tama:brandslide", { detail: next })); }
      }
      draw();
      if (p < 1) anim = requestAnimationFrame(step); else { anim = null; annoAlpha = 1; draw(); }
    };
    anim = requestAnimationFrame(step);
  }

  function slideShow() {
    if (slideTimer) clearInterval(slideTimer);
    slideTimer = setInterval(() => { if ((mode === "brand" || mode === "ai") && !anim && !drag) slideTo(); }, 3400);
  }

  function jumpTo(k) {
    if (mode === "device") { if (k !== groupIndex) cycleTo(k); }
    else if (mode === "brand") { if (k !== brandIndex) slideTo(k); }
    else if (mode === "ai") { if (k !== aiIndex) slideTo(k); }
  }

  function setMode(id) {
    mode = id;
    selectTab(tabs, "tab", id);
    mount.classList.add("free");
    if (slideTimer) { clearInterval(slideTimer); slideTimer = null; }
    stopCarousel();
    annoAlpha = 1;

    if (id === "device") { groupIndex = 0; buildDots(GROUPS.length); setDot(0); startCarousel(); }
    if (id === "brand") { brandIndex = 0; buildDots(BRAND_SCREENS.length); setDot(0); slideShow(); }
    if (id === "ai") { aiIndex = 0; buildDots(AI_SCREENS.length); setDot(0); slideShow(); }
    if (id === "games") { gamesIndex = 0; buildDots(0); slideTimer = setInterval(() => { if (mode === "games" && !anim && !drag) { gamesIndex++; draw(); } }, 620); }
    if (cap) cap.textContent = id === "device" ? GROUPS[0].caption : id === "brand" ? BRAND_SCREENS[0].caption : id === "ai" ? AI_SCREENS[0].caption : MODES[id].caption;

    const m = MODES[id];
    const startYaw = (id === "brand" || id === "ai") ? slideYaw(id, 0) : m.yaw;
    animateTo(startYaw, m.pitch);
    window.dispatchEvent(new CustomEvent("tama:mode", { detail: id }));
  }

  tabs.forEach((b) => b.addEventListener("click", () => setMode(b.dataset.tab)));

  svg.addEventListener("pointerdown", (e) => {
    if (e.target && e.target.closest && e.target.closest(".btn-a")) {
      pressButton((v) => { pressK = v; });
      window.dispatchEvent(new CustomEvent("tama:accent-next"));
      return;
    }
    if (e.target && e.target.closest && e.target.closest(".btn-side")) { pressButton((v) => { pressS = v; }); return; }
    if (anim) { cancelAnimationFrame(anim); anim = null; annoAlpha = 1; }
    drag = { x: e.clientX, y: e.clientY, yaw, pitch };
    svg.setPointerCapture(e.pointerId);
    mount.classList.add("grabbing");
  });
  svg.addEventListener("pointermove", (e) => {
    if (!drag) return;
    yaw = Math.max(YAW_MIN, Math.min(YAW_MAX, drag.yaw + (e.clientX - drag.x) * 0.4));
    pitch = Math.max(PITCH_MIN, Math.min(PITCH_MAX, drag.pitch - (e.clientY - drag.y) * 0.35));
    draw();
  });
  const end = (e) => { if (drag) { mount.classList.remove("grabbing"); drag = null; try { svg.releasePointerCapture(e.pointerId); } catch (_) {} } };
  svg.addEventListener("pointerup", end);
  svg.addEventListener("pointercancel", end);

  setMode("brand");
}
