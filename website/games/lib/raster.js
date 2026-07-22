export function tracePath(ctx, pts) {
  ctx.beginPath();
  ctx.moveTo(pts[0].x, pts[0].y);
  for (let i = 1; i < pts.length; i++) ctx.lineTo(pts[i].x, pts[i].y);
  ctx.closePath();
}

function uvTransform(ctx, p0, p1, p2, uv0, uv1, uv2) {
  const du1 = uv1[0] - uv0[0], dv1 = uv1[1] - uv0[1];
  const du2 = uv2[0] - uv0[0], dv2 = uv2[1] - uv0[1];
  const det = du1 * dv2 - dv1 * du2;
  if (!det) return false;
  const dx1 = p1.x - p0.x, dy1 = p1.y - p0.y;
  const dx2 = p2.x - p0.x, dy2 = p2.y - p0.y;
  const a = (dx1 * dv2 - dx2 * dv1) / det;
  const b = (dy1 * dv2 - dy2 * dv1) / det;
  const c = (dx2 * du1 - dx1 * du2) / det;
  const d = (dy2 * du1 - dy1 * du2) / det;
  ctx.transform(a, b, c, d, p0.x - a * uv0[0] - c * uv0[1], p0.y - b * uv0[0] - d * uv0[1]);
  return true;
}

export function affineTri(ctx, img, p0, p1, p2, uv0, uv1, uv2) {
  ctx.save();
  tracePath(ctx, [p0, p1, p2]);
  ctx.clip();
  if (uvTransform(ctx, p0, p1, p2, uv0, uv1, uv2)) {
    ctx.drawImage(img, 0, 0, img.width, img.height,
                  -0.5, -0.5, img.width + 1, img.height + 1);
  }
  ctx.restore();
}

export function texQuadOne(ctx, img, proj, uvs) {
  ctx.save();
  tracePath(ctx, proj);
  ctx.clip();
  if (uvTransform(ctx, proj[0], proj[1], proj[3], uvs[0], uvs[1], uvs[3])) {
    ctx.drawImage(img, 0, 0, img.width, img.height,
                  -0.5, -0.5, img.width + 1, img.height + 1);
  }
  ctx.restore();
}

export function texPoly(ctx, img, proj, uvs) {
  for (let i = 2; i < proj.length; i++) {
    affineTri(ctx, img, proj[0], proj[i - 1], proj[i], uvs[0], uvs[i - 1], uvs[i]);
  }
}
