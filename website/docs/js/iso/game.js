import { rad, project } from "./core.js";

const A = rad(-32), E = rad(27);
const SIL = "#1b1d23", GRID = "#e2e5eb", CALL = "#aeb4be", DIMTEXT = "#9298a4", BACK = "#d5d8de";
const TOP = "#f4f5f8", SOUTH = "#e0e3e8", EAST = "#d0d4db";

const S = 44;
const GRID_X = 8, GRID_Y = 5;
const CUBE_CELL = [5, 1];
const ROLL = rad(34);

const FACES = [
  [4, 5, 6, 7],
  [0, 3, 2, 1],
  [3, 2, 6, 7],
  [0, 1, 5, 4],
  [1, 2, 6, 5],
  [0, 4, 7, 3],
];
const EDGES = [
  [0, 1], [1, 2], [2, 3], [3, 0],
  [4, 5], [5, 6], [6, 7], [7, 4],
  [0, 4], [1, 5], [2, 6], [3, 7],
];

const pt = (p) => `${p.x.toFixed(1)},${p.y.toFixed(1)}`;
const P = (v) => project(v, A, E);
const poly = (pts, fill, extra = "") => `<polygon points="${pts.map(pt).join(" ")}" fill="${fill}" ${extra}/>`;
const dashed = (pts, w = 1.1) =>
  `<path d="${pts.map((q, i) => `${i ? "L" : "M"}${pt(q)}`).join(" ")}" fill="none" stroke="${CALL}" stroke-width="${w}" stroke-dasharray="5 4"/>`;
const edgeKey = (a, b) => (a < b ? `${a}-${b}` : `${b}-${a}`);
const sub3 = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const cross3 = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const dot3 = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

function box(x0, y0, w, d, h) {
  return [
    [x0, y0, 0], [x0 + w, y0, 0], [x0 + w, y0 + d, 0], [x0, y0 + d, 0],
    [x0, y0, h], [x0 + w, y0, h], [x0 + w, y0 + d, h], [x0, y0 + d, h],
  ];
}

function outwardNormal(verts, idx) {
  const a = verts[idx[0]], b = verts[idx[1]], c = verts[idx[2]];
  let n = cross3(sub3(b, a), sub3(c, a));
  const mid = [
    (verts[idx[0]][0] + verts[idx[1]][0] + verts[idx[2]][0] + verts[idx[3]][0]) / 4,
    (verts[idx[0]][1] + verts[idx[1]][1] + verts[idx[2]][1] + verts[idx[3]][1]) / 4,
    (verts[idx[0]][2] + verts[idx[1]][2] + verts[idx[2]][2] + verts[idx[3]][2]) / 4,
  ];
  const center = [
    (verts[0][0] + verts[6][0]) / 2,
    (verts[0][1] + verts[6][1]) / 2,
    (verts[0][2] + verts[6][2]) / 2,
  ];
  if (dot3(n, sub3(mid, center)) < 0) n = [-n[0], -n[1], -n[2]];
  return n;
}

function isFront(verts, idx) {
  const n = outwardNormal(verts, idx);
  return project(n, A, E).depth - project([0, 0, 0], A, E).depth > 0;
}

function faceFill(n, topFill) {
  const ax = Math.abs(n[0]), ay = Math.abs(n[1]), az = Math.abs(n[2]);
  if (az >= ax && az >= ay) return topFill;
  if (ay >= ax) return SOUTH;
  return EAST;
}

function solidBox(verts, topFill = TOP) {
  const p = verts.map(P);
  const frontFaces = FACES.map((idx) => ({ idx, n: outwardNormal(verts, idx), front: isFront(verts, idx) }));
  const frontEdges = new Set();
  for (const { idx, front } of frontFaces) {
    if (!front) continue;
    idx.forEach((j, k) => frontEdges.add(edgeKey(j, idx[(k + 1) % idx.length])));
  }

  let out = "";

  for (const [a, b] of EDGES) {
    if (frontEdges.has(edgeKey(a, b))) continue;
    out += `<line x1="${p[a].x.toFixed(1)}" y1="${p[a].y.toFixed(1)}" x2="${p[b].x.toFixed(1)}" y2="${p[b].y.toFixed(1)}" stroke="${BACK}" stroke-width="0.75" stroke-linecap="round"/>`;
  }

  for (const { idx, n, front } of frontFaces) {
    if (!front) continue;
    out += poly(idx.map((j) => p[j]), faceFill(n, topFill));
  }

  for (const [a, b] of EDGES) {
    if (!frontEdges.has(edgeKey(a, b))) continue;
    out += `<line x1="${p[a].x.toFixed(1)}" y1="${p[a].y.toFixed(1)}" x2="${p[b].x.toFixed(1)}" y2="${p[b].y.toFixed(1)}" stroke="${SIL}" stroke-width="0.75" stroke-linecap="round"/>`;
  }
  return out;
}

function arrowhead(e0, e1, size = 7) {
  const vl = Math.hypot(e1.x - e0.x, e1.y - e0.y) || 1;
  const ux = (e1.x - e0.x) / vl, uy = (e1.y - e0.y) / vl;
  const side = size * 0.48;
  const tip = [
    { x: e1.x, y: e1.y },
    { x: e1.x - size * ux - side * uy, y: e1.y - size * uy + side * ux },
    { x: e1.x - size * ux + side * uy, y: e1.y - size * uy - side * ux },
  ];
  return poly(tip, CALL);
}

const dashedArrow = (pts) => dashed(pts) + arrowhead(pts[pts.length - 2], pts[pts.length - 1]);

function grid() {
  let out = "";
  const line = (a, b) =>
    `<line x1="${a.x.toFixed(1)}" y1="${a.y.toFixed(1)}" x2="${b.x.toFixed(1)}" y2="${b.y.toFixed(1)}" stroke="${GRID}" stroke-width="1"/>`;
  for (let i = 0; i <= GRID_X; i++) out += line(P([i * S, 0, 0]), P([i * S, (GRID_Y - 1) * S, 0]));
  for (let j = 0; j < GRID_Y; j++) out += line(P([0, j * S, 0]), P([GRID_X * S, j * S, 0]));
  return out;
}

function rollingCube() {
  const cx = CUBE_CELL[0] * S, cy = CUBE_CELL[1] * S;
  const pivotX = cx + S;
  const cos = Math.cos(ROLL), sin = Math.sin(ROLL);
  const verts = box(cx, cy, S, S, S).map(([x, y, z]) => {
    const dx = x - pivotX;
    return [pivotX + dx * cos + z * sin, y, -dx * sin + z * cos];
  });

  const ghostVerts = box(cx - S, cy, S, S, S);
  const ghost = ghostVerts.map(P);
  let out = "";
  for (const idx of FACES) {
    if (!isFront(ghostVerts, idx)) continue;
    out += poly(idx.map((i) => ghost[i]), "none", `stroke="${CALL}" stroke-width="0.75" stroke-dasharray="4 4" stroke-linejoin="round"`);
  }

  const arc = [];
  for (let k = 0; k <= 18; k++) {
    const u = k / 18;
    const x = cx - 0.9 * S + u * 1.4 * S;
    const z = S * 1.7 + S * 0.25 * Math.sin(Math.PI * u);
    arc.push(P([x, cy + S / 2, z]));
  }
  return out + solidBox(verts) + dashedArrow(arc);
}

const HOLD = { x: 118, y: 22, tilt: 12, w: 100, h: 50 };
const SKIN = "#f4f5f8";

function thumb(side) {
  const inner = `<rect x="-56" y="8" width="12" height="28" rx="6" transform="rotate(28 -50 22)" fill="${SKIN}" stroke="${SIL}" stroke-width="0.75"/>`;
  return side === 1 ? `<g transform="scale(-1 1)">${inner}</g>` : inner;
}

function deviceFace() {
  const { w, h } = HOLD;
  const x0 = -w / 2, y0 = -h / 2;
  let out = `<rect x="${x0}" y="${y0}" width="${w}" height="${h}" rx="9" fill="#f0f2f5" stroke="${SIL}" stroke-width="0.75"/>`;

  const sw = 0.55 * w, sh = 0.62 * h;
  const sx = x0 + 0.09 * w, sy = -sh / 2;
  out += `<rect x="${sx.toFixed(1)}" y="${sy.toFixed(1)}" width="${sw.toFixed(1)}" height="${sh.toFixed(1)}" rx="3" fill="#fafbfc" stroke="${SIL}" stroke-width="0.75"/>`;

  const q = 8;
  const qx = sx + 8, qy = -q / 2;
  out += `<rect x="${qx.toFixed(1)}" y="${qy.toFixed(1)}" width="${q}" height="${q}" fill="none" stroke="${SIL}" stroke-width="0.7"/>`;
  const ax0 = qx + q + 5, ax1 = sx + sw - 9, ay = 0;
  out += `<line x1="${ax0.toFixed(1)}" y1="${ay}" x2="${(ax1 - 4.5).toFixed(1)}" y2="${ay}" stroke="${CALL}" stroke-width="0.9" stroke-dasharray="3 2.5"/>`;
  out += arrowhead({ x: ax1 - 4.5, y: ay }, { x: ax1, y: ay }, 4.5);

  const bw = 0.066 * w, bh = 0.53 * h;
  out += `<rect x="${(0.8 * w + x0 - bw / 2).toFixed(1)}" y="${(-bh / 2).toFixed(1)}" width="${bw.toFixed(1)}" height="${bh.toFixed(1)}" rx="${(bw / 2).toFixed(1)}" fill="color-mix(in srgb, var(--accent) 52%, #fff)" stroke="${SIL}" stroke-width="0.75"/>`;
  return out;
}

function tiltArc() {
  const r = 50, pts = [];
  for (let k = 0; k <= 14; k++) {
    const t = rad(-118 + (52 * k) / 14);
    pts.push({ x: r * Math.cos(t), y: r * Math.sin(t) + 16 });
  }
  return dashedArrow(pts);
}

function heldDevice() {
  const inner = deviceFace() + thumb(-1) + thumb(1) + tiltArc();
  let out = `<g transform="translate(${HOLD.x} ${HOLD.y}) rotate(${HOLD.tilt})">${inner}</g>`;
  out += `<text x="${HOLD.x}" y="${HOLD.y + 58}" font-size="10.5" letter-spacing="1.5" fill="${DIMTEXT}" text-anchor="middle">TILT TO ROLL</text>`;
  return out;
}

export function initGameMock() {
  const host = document.querySelector(".gamemock-screen");
  if (!host) return;
  host.innerHTML = `<svg viewBox="-14 -112 420 194" role="img" aria-label="Two hands tilt the device right and the cube on the grid rolls right">${grid() + rollingCube() + heldDevice()}</svg>`;
}
