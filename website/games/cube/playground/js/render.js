import { Camera, Follower, sub, dot, cross, normOf } from "../../../lib/math3d.js";
import { tracePath } from "../../../lib/raster.js";
import { FACES, rollingVerts } from "../../roller.js";

const FOV = 1.9;
const CAM_BACK = 5.4;
const CAM_UP = 4.2;
const CUBE = 0.92;
const GRID = 14;
const FADE_NEAR = 4;
const FADE_FAR = 15;

const ink = (alpha) => `rgba(74,246,138,${alpha})`;

const fade = (z) => 1 - Math.max(0, Math.min(1, (z - FADE_NEAR) / (FADE_FAR - FADE_NEAR)));

function gridLine(ctx, cam, a, b, alpha) {
  const steps = 8;
  ctx.beginPath();
  let drawing = false;
  for (let i = 0; i <= steps; i++) {
    const t = i / steps;
    const p = cam.project([a[0] + (b[0] - a[0]) * t, 0, a[2] + (b[2] - a[2]) * t]);
    if (!p) {
      drawing = false;
      continue;
    }
    if (drawing) ctx.lineTo(p.x, p.y);
    else ctx.moveTo(p.x, p.y);
    drawing = true;
  }
  ctx.strokeStyle = ink(alpha);
  ctx.stroke();
}

function drawGrid(ctx, cam, center) {
  ctx.lineWidth = 1;
  const cx = Math.round(center.x);
  const cy = Math.round(center.y);
  for (let i = -GRID; i <= GRID; i++) {
    const mid = cam.project([cx + i, 0, cy]);
    const alpha = 0.28 * (mid ? fade(mid.z) : 1);
    gridLine(ctx, cam, [cx + i, 0, cy - GRID], [cx + i, 0, cy + GRID], alpha);
    gridLine(ctx, cam, [cx - GRID, 0, cy + i], [cx + GRID, 0, cy + i], alpha);
  }
}

function drawCube(ctx, cam, verts) {
  FACES.map((idx) => {
    const pts = idx.map((i) => verts[i]);
    const center = pts.reduce((acc, p) => [acc[0] + p[0] / 4, acc[1] + p[1] / 4,
                                           acc[2] + p[2] / 4], [0, 0, 0]);
    let normal = normOf(cross(sub(pts[1], pts[0]), sub(pts[2], pts[0])));
    const centroid = verts.reduce((acc, p) => [acc[0] + p[0] / 8, acc[1] + p[1] / 8,
                                               acc[2] + p[2] / 8], [0, 0, 0]);
    if (dot(normal, sub(center, centroid)) < 0) normal = normal.map((v) => -v);
    return { pts, normal, depth: cam.depth(center) };
  })
    .filter((f) => dot(f.normal, sub(cam.pos, f.pts[0])) > 0)
    .sort((a, b) => b.depth - a.depth)
    .forEach((f) => {
      const proj = f.pts.map((p) => cam.project(p));
      if (proj.some((p) => !p)) return;
      const lit = 0.1 + 0.16 * Math.max(0, dot(f.normal, normOf([0.35, 0.9, 0.4])));
      tracePath(ctx, proj);
      ctx.fillStyle = ink(lit);
      ctx.fill();
      ctx.lineWidth = 1.6;
      ctx.strokeStyle = ink(0.95);
      ctx.stroke();
    });
}

const follower = new Follower();

export function render(ctx, w, h, world, time) {
  ctx.fillStyle = "#020803";
  ctx.fillRect(0, 0, w, h);

  const c = world.center();
  const eye = follower.update(c, time);
  const cam = new Camera(w, h, [eye.x, 0.45, eye.y], CAM_BACK, CAM_UP, FOV);

  drawGrid(ctx, cam, c);

  const inset = (1 - CUBE) / 2;
  ctx.save();
  ctx.shadowColor = ink(0.7);
  ctx.shadowBlur = 14;
  drawCube(ctx, cam, rollingVerts(world.col + inset, world.row + inset, CUBE, world.roll));
  ctx.restore();

  const scan = ctx.createLinearGradient(0, 0, 0, h);
  scan.addColorStop(0, "rgba(0,0,0,0.32)");
  scan.addColorStop(0.5, "rgba(0,0,0,0)");
  scan.addColorStop(1, "rgba(0,0,0,0.38)");
  ctx.fillStyle = scan;
  ctx.fillRect(0, 0, w, h);
}
