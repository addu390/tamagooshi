import { rgbOf, mixRgb, css } from "../../../lib/color.js";
import { normOf, sub, dot, lerp3 } from "../../../lib/math3d.js";
import { tracePath, affineTri, texPoly, texQuadOne } from "../../../lib/raster.js";
import { C } from "./palette.js";

export const LIGHT = normOf([0.3, 0.9, 0.42]);

const FOG_NEAR = 4;
const FOG_FAR = 16;
const FOG_RGB = rgbOf(C.fog);

export const HIDDEN = 0.97;

export const fogT = (z) =>
  Math.max(0, Math.min(1, (z - FOG_NEAR) / (FOG_FAR - FOG_NEAR)));

const fogRgba = (t) => `rgba(${FOG_RGB[0]},${FOG_RGB[1]},${FOG_RGB[2]},${t})`;

export const shadeColor = (darkRgb, brightRgb, lit, fogAmount) =>
  css(mixRgb(mixRgb(darkRgb, brightRgb, lit), FOG_RGB, fogAmount));

export const quadLit = (normal) => Math.max(0, dot(normal, LIGHT));

export const UV_TILE = (img) => [[0, 0], [img.width, 0], [img.width, img.height], [0, img.height]];

export function shadePath(ctx, proj, dark, fogAmount) {
  tracePath(ctx, proj);
  if (dark > 0.01) {
    ctx.fillStyle = `rgba(0,0,0,${Math.min(1, dark)})`;
    ctx.fill();
  }
  if (fogAmount > 0.01) {
    ctx.fillStyle = fogRgba(fogAmount);
    ctx.fill();
  }
}

export function drawTexQuadWorld(ctx, cam, img, wpts, uvs, dark, flat = false) {
  const proj = wpts.map((p) => cam.project(p));
  if (proj.some((p) => !p)) return;
  const fogAmount = fogT((proj[0].z + proj[2].z) / 2);
  if (fogAmount >= HIDDEN) return;
  if (flat && proj.length === 4) texQuadOne(ctx, img, proj, uvs);
  else texPoly(ctx, img, proj, uvs);
  shadePath(ctx, proj, dark, fogAmount);
}

export function drawQuads(ctx, cam, quads) {
  quads
    .filter((q) => dot(q.normal, sub(cam.pos, q.pts[0])) > 0)
    .map((q) => {
      const center = q.pts.reduce((acc, p) => [acc[0] + p[0] / q.pts.length,
                                               acc[1] + p[1] / q.pts.length,
                                               acc[2] + p[2] / q.pts.length], [0, 0, 0]);
      return { ...q, depth: cam.depth(center) };
    })
    .sort((a, b) => ((a.layer ?? 0) - (b.layer ?? 0)) || (b.depth - a.depth))
    .forEach((q) => {
      const dark = (1 - quadLit(q.normal)) * (q.shade ?? 0.5) + (q.extraDark ?? 0);
      drawTexQuadWorld(ctx, cam, q.img, q.pts, q.uvs ?? UV_TILE(q.img), dark, q.flat);
    });
}

export function texQuad(ctx, cam, img, wpts, { dark = 0, subdivide = 1 } = {}) {
  const n = subdivide;
  for (let i = 0; i < n; i++) {
    for (let j = 0; j < n; j++) {
      const u0 = i / n, u1 = (i + 1) / n;
      const v0 = j / n, v1 = (j + 1) / n;
      const at = (u, v) => lerp3(lerp3(wpts[0], wpts[1], u), lerp3(wpts[3], wpts[2], u), v);
      const corners = [at(u0, v0), at(u1, v0), at(u1, v1), at(u0, v1)];
      const proj = corners.map((p) => cam.project(p));
      if (proj.some((p) => !p)) continue;
      const fogAmount = fogT((proj[0].z + proj[2].z) / 2);
      if (fogAmount >= HIDDEN) continue;
      const uv = [[u0, v0], [u1, v0], [u1, v1], [u0, v1]]
        .map(([u, v]) => [u * img.width, v * img.height]);
      affineTri(ctx, img, proj[0], proj[1], proj[2], uv[0], uv[1], uv[2]);
      affineTri(ctx, img, proj[0], proj[2], proj[3], uv[0], uv[2], uv[3]);
      shadePath(ctx, proj, dark, fogAmount);
    }
  }
}

export function tiledWall(quads, img, origin, uDir, vDir, uLen, vLen, tileU, tileV,
                          normal, shade) {
  const at = (u, v) => [
    origin[0] + uDir[0] * u + vDir[0] * v,
    origin[1] + uDir[1] * u + vDir[1] * v,
    origin[2] + uDir[2] * u + vDir[2] * v,
  ];
  for (let u = 0; u < uLen - 1e-6; u += tileU) {
    const uw = Math.min(tileU, uLen - u);
    for (let v = 0; v < vLen - 1e-6; v += tileV) {
      const vh = Math.min(tileV, vLen - v);
      const uPix = (uw / tileU) * img.width;
      const vPix = (vh / tileV) * img.height;
      quads.push({
        img, normal, shade,
        pts: [at(u, v), at(u + uw, v), at(u + uw, v + vh), at(u, v + vh)],
        uvs: [[0, img.height], [uPix, img.height],
              [uPix, img.height - vPix], [0, img.height - vPix]],
      });
    }
  }
}

const SHADOW_TILT = [-LIGHT[0] / LIGHT[1], -LIGHT[2] / LIGHT[1]];

export function softShadow(ctx, cam, [x, y, z], objH, rx, rz, alpha) {
  const p = cam.project([x + SHADOW_TILT[0] * objH * 0.5, y + 0.012,
                         z + SHADOW_TILT[1] * objH * 0.5]);
  if (!p) return;
  const fogAmount = fogT(p.z);
  if (fogAmount >= HIDDEN) return;
  const rxS = cam.f * rx / p.z;
  const rzS = cam.f * rz / p.z;
  const a = alpha * (1 - fogAmount);
  ctx.save();
  ctx.translate(p.x, p.y);
  ctx.scale(1, rzS / rxS);
  const g = ctx.createRadialGradient(0, 0, 0, 0, 0, rxS);
  g.addColorStop(0, `rgba(0,0,0,${a})`);
  g.addColorStop(0.6, `rgba(0,0,0,${a * 0.82})`);
  g.addColorStop(1, "rgba(0,0,0,0)");
  ctx.fillStyle = g;
  ctx.fillRect(-rxS, -rxS, rxS * 2, rxS * 2);
  ctx.restore();
}
