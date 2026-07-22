import { rgbOf } from "../../../lib/color.js";
import { normOf, sub, dot, cross } from "../../../lib/math3d.js";
import { tracePath, texPoly } from "../../../lib/raster.js";
import { FACES } from "../../core/roller.js";
import { LIGHT, HIDDEN, fogT, shadeColor, quadLit, shadePath, UV_TILE } from "./scene.js";

const GOLD_DARK = rgbOf("#5c3c12");
const GOLD_BRIGHT = rgbOf("#cfa14e");
const GOLD_SPEC = rgbOf("#e5c47c");

export function boxFaces(x0, x1, y0, y1, z0, z1) {
  const v = [
    [x0, y0, z0], [x1, y0, z0], [x1, y0, z1], [x0, y0, z1],
    [x0, y1, z0], [x1, y1, z0], [x1, y1, z1], [x0, y1, z1],
  ];
  return [
    { pts: [v[4], v[5], v[6], v[7]], normal: [0, 1, 0] },
    { pts: [v[1], v[0], v[4], v[5]], normal: [0, 0, -1] },
    { pts: [v[3], v[2], v[6], v[7]], normal: [0, 0, 1] },
    { pts: [v[0], v[3], v[7], v[4]], normal: [-1, 0, 0] },
    { pts: [v[2], v[1], v[5], v[6]], normal: [1, 0, 0] },
  ];
}

export function drawCrate(ctx, cam, img, verts, crashed) {
  const centroid = verts.reduce((acc, p) => [acc[0] + p[0] / 8, acc[1] + p[1] / 8,
                                             acc[2] + p[2] / 8], [0, 0, 0]);

  FACES.map((idx) => {
    const pts = idx.map((i) => verts[i]);
    const center = pts.reduce((acc, p) => [acc[0] + p[0] / 4, acc[1] + p[1] / 4,
                                           acc[2] + p[2] / 4], [0, 0, 0]);
    let normal = normOf(cross(sub(pts[1], pts[0]), sub(pts[2], pts[0])));
    if (dot(normal, sub(center, centroid)) < 0) normal = normal.map((v) => -v);
    return { pts, normal, depth: cam.depth(center) };
  })
    .filter((f) => dot(f.normal, sub(cam.pos, f.pts[0])) > 0)
    .sort((a, b) => b.depth - a.depth)
    .forEach((f) => {
      const proj = f.pts.map((p) => cam.project(p));
      if (proj.some((p) => !p)) return;
      const fogAmount = fogT((proj[0].z + proj[2].z) / 2);
      texPoly(ctx, img, proj, UV_TILE(img));
      shadePath(ctx, proj, (1 - quadLit(f.normal)) * 0.55, fogAmount);
      if (crashed) {
        tracePath(ctx, proj);
        ctx.fillStyle = "rgba(255,72,48,0.4)";
        ctx.fill();
      }
    });
}

const COIN_SEGS = 24;
const COIN_R = 0.26;
const COIN_T = 0.09;

function coinRing(center, u, n, offset, scale = 1) {
  const pts = [];
  for (let i = 0; i < COIN_SEGS; i++) {
    const ang = (i / COIN_SEGS) * Math.PI * 2;
    const rad = COIN_R * scale;
    const dx = u[0] * Math.cos(ang) * rad + n[0] * offset;
    const dy = Math.sin(ang) * rad;
    const dz = u[2] * Math.cos(ang) * rad + n[2] * offset;
    pts.push([center[0] + dx, center[1] + dy, center[2] + dz]);
  }
  return pts;
}

export function drawCoin(ctx, cam, tex, center, time) {
  const fogAmount = fogT(cam.depth(center));
  if (fogAmount >= HIDDEN) return;

  const spin = time * 2.2 + center[2] * 0.6;
  const u = [Math.cos(spin), 0, Math.sin(spin)];
  const n = [-Math.sin(spin), 0, Math.cos(spin)];
  const facing = dot(n, sub(cam.pos, center)) > 0 ? 1 : -1;
  const half = COIN_T / 2;

  const front = coinRing(center, u, n, half * facing);
  const back = coinRing(center, u, n, -half * facing);

  const rim = [];
  for (let i = 0; i < COIN_SEGS; i++) {
    const j = (i + 1) % COIN_SEGS;
    const quad = [front[i], front[j], back[j], back[i]];
    const qc = quad.reduce((acc, p) => [acc[0] + p[0] / 4, acc[1] + p[1] / 4,
                                        acc[2] + p[2] / 4], [0, 0, 0]);
    const off = sub(qc, center);
    const along = dot(off, n);
    const radial = normOf([off[0] - n[0] * along, off[1], off[2] - n[2] * along]);
    if (dot(radial, sub(cam.pos, qc)) <= 0) continue;
    rim.push({ quad, depth: cam.depth(qc), lit: Math.max(0, dot(radial, LIGHT)) });
  }

  rim.sort((a, b) => b.depth - a.depth).forEach(({ quad, lit }) => {
    const proj = quad.map((p) => cam.project(p));
    if (proj.some((p) => !p)) return;
    ctx.fillStyle = shadeColor(GOLD_DARK, GOLD_BRIGHT, 0.15 + 0.85 * lit, fogAmount);
    tracePath(ctx, proj);
    ctx.fill();
  });

  const cap = front.map((p) => cam.project(p));
  if (cap.some((p) => !p)) return;

  const capLit = Math.abs(dot(n, LIGHT));
  const cx = cap.reduce((s, p) => s + p.x, 0) / COIN_SEGS;
  const cy = cap.reduce((s, p) => s + p.y, 0) / COIN_SEGS;
  const cr = cap.reduce((s, p) => Math.max(s, Math.hypot(p.x - cx, p.y - cy)), 0);
  const g = ctx.createRadialGradient(cx - cr * 0.3, cy - cr * 0.38, cr * 0.16,
                                     cx, cy, cr * 1.02);
  g.addColorStop(0, shadeColor(GOLD_BRIGHT, GOLD_SPEC, 0.35 + 0.4 * capLit, fogAmount));
  g.addColorStop(0.55, shadeColor(GOLD_DARK, GOLD_BRIGHT, 0.35 + 0.5 * capLit, fogAmount));
  g.addColorStop(1, shadeColor(GOLD_DARK, GOLD_BRIGHT, 0.15 + 0.3 * capLit, fogAmount));
  tracePath(ctx, cap);
  ctx.fillStyle = g;
  ctx.fill();
  ctx.save();
  tracePath(ctx, cap);
  ctx.clip();
  ctx.globalAlpha = 0.18 * (1 - fogAmount);
  ctx.globalCompositeOperation = "multiply";
  ctx.drawImage(tex.rock, 0, 0, 64, 64, cx - cr, cy - cr, cr * 2, cr * 2);
  ctx.restore();

  const emboss = (scale, dy, alpha) => {
    const ring = coinRing(center, u, n, half * facing * 1.01, scale)
      .map((p) => cam.project(p));
    if (ring.some((p) => !p)) return;
    ctx.save();
    ctx.translate(0, dy);
    tracePath(ctx, ring);
    ctx.strokeStyle = `rgba(${dy < 0 ? "58,37,10" : "222,192,130"},${alpha * (1 - fogAmount)})`;
    ctx.lineWidth = Math.max(1, cr * 0.05);
    ctx.stroke();
    ctx.restore();
  };
  emboss(0.62, -Math.max(0.6, cr * 0.03), 0.5);
  emboss(0.62, Math.max(0.6, cr * 0.03), 0.4);
}
