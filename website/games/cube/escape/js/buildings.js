import { normOf } from "../../../lib/math3d.js";
import { tracePath } from "../../../lib/raster.js";
import { slotZ, SLOTS, MAX_SETBACK, KERB_H, STORY_H } from "./village.js";
import { fogT, HIDDEN, tiledWall } from "./scene.js";
import { boxFaces } from "./props.js";

const WALL_TILE = 0.675;
const WALL_SHADE = 0.38;
const CORNER_W = 0.3;
const ROOF_TILE = 0.75;
const ROOF_RUN = 0.8;
const ROOF_RISE = 1.5;
const DOOR_W = 0.62;
const DOOR_H = 1.24;
const WIN = 0.55;

function timberStory(quads, f, tex, y, length, normal, xFront) {
  const win = f.lit[f.windowParity] ? tex.window_lit : tex.window;
  const count = Math.round(length / WALL_TILE);
  const tile = length / count;
  for (let i = 0; i < count; i++) {
    const isWindow = i % 2 === f.windowParity && i > 0 && i < count - 1;
    const img = isWindow ? win : tex[f.upper];
    const z = f.z0 + i * tile;
    quads.push({
      img, normal, shade: WALL_SHADE,
      pts: [[xFront(0), y, z], [xFront(0), y, z + tile],
            [xFront(0), y + STORY_H, z + tile], [xFront(0), y + STORY_H, z]],
      uvs: [[0, img.height], [img.width, img.height], [img.width, 0], [0, 0]],
    });
  }
}

function dormerQuads(quads, f, tex, zc, eaveY, xEave, street) {
  const w = 0.72;
  const slopeXAtY = (y) => xEave - street * ROOF_RUN * ((y - eaveY) / ROOF_RISE);
  const yB = eaveY + 0.24;
  const yT = yB + 0.54;
  const yA = yT + 0.32;
  const xF = xEave - street * 0.02;
  const xSb = slopeXAtY(yB);
  const xSt = slopeXAtY(yT);
  const xR = slopeXAtY(yA);
  const win = tex.window_lit;
  const wall = tex.frame_clay;
  const roofImg = f.roof === "thatch" ? tex.thatch : tex[f.roof];
  const normal = [street, 0, 0];

  quads.push({
    img: wall, normal, shade: WALL_SHADE, layer: 2,
    pts: [[xF, yB, zc - w / 2], [xF, yB, zc + w / 2],
          [xF, yT, zc + w / 2], [xF, yT, zc - w / 2]],
    uvs: [[14, 50], [50, 50], [50, 18], [14, 18]],
  });
  const ww = 0.48;
  const wx = xF + street * 0.012;
  quads.push({
    img: win, normal, shade: WALL_SHADE, layer: 3,
    pts: [[wx, yB + 0.05, zc - ww / 2], [wx, yB + 0.05, zc + ww / 2],
          [wx, yT - 0.05, zc + ww / 2], [wx, yT - 0.05, zc - ww / 2]],
    uvs: [[0, win.height], [win.width, win.height], [win.width, 0], [0, 0]],
  });
  quads.push({
    img: wall, normal, shade: WALL_SHADE, layer: 2,
    pts: [[xF, yT, zc - w / 2], [xF, yT, zc + w / 2], [xF, yA, zc]],
    uvs: [[14, 50], [50, 50], [32, 20]],
  });
  for (const dz of [-1, 1]) {
    quads.push({
      img: wall, normal: [0, 0, dz], shade: 0.3, layer: 2,
      pts: [[xF, yB, zc + dz * w / 2], [xF, yT, zc + dz * w / 2],
            [xSt, yT, zc + dz * w / 2], [xSb, yB, zc + dz * w / 2]],
      uvs: [[14, 50], [14, 16], [40, 16], [46, 50]],
    });
    quads.push({
      img: roofImg, layer: 3, shade: 0.4,
      normal: normOf([street * 0.3, 0.55, dz * 0.85]),
      pts: [[xF, yT, zc + dz * (w / 2 + 0.05)],
            [xF, yA, zc],
            [xR, yA, zc],
            [xSt, yT, zc + dz * (w / 2 + 0.05)]],
      uvs: [[0, 44], [0, 0], [52, 0], [52, 44]],
    });
  }
}

export function crateBoxes(f) {
  if (!f.crates) return [];
  const street = -f.side;
  const big = 0.46;
  const small = big * 0.64;
  const freeSlots = [0, 1, 2].filter((i) => i !== f.doorSlot);
  const slot = freeSlots[Math.floor(f.crates.at * freeSlots.length) % freeSlots.length];
  const inward = f.crates.at > 0.5 ? -1 : 1;
  const cz = f.crates.beside
    ? (f.crates.at > 0.5 ? f.z1 - 0.4 : f.z0 + 0.4)
    : slotZ(f, slot);
  const back = f.xWall + street * (0.05 + (f.crates.beside ? 0 : f.crates.off));
  const span = (size) => [Math.min(back, back + street * size),
                          Math.max(back, back + street * size)];

  const boxes = [];
  const add = (size, y0, zc, kind) => {
    const [x0, x1] = span(size);
    boxes.push({ x0, x1, y0, size, zc, kind: f.crates.kinds[kind] });
  };
  add(big, KERB_H, cz, 0);
  if (f.crates.stack >= 1) add(small, KERB_H + big, cz + inward * 0.04, 1);
  if (f.crates.stack === 2) add(big, KERB_H, cz + inward * (big + 0.03), 2);
  return boxes;
}

export function facadeQuads(f, tex) {
  const quads = [];
  const street = -f.side;
  const x = f.xWall;
  const y0 = KERB_H;
  const wallTop = y0 + f.stories * STORY_H;
  const length = f.z1 - f.z0;
  const normal = [street, 0, 0];
  const xFront = (lift) => x + street * lift;
  const timberUpper = f.kind === "timber" || f.kind === "cottage";

  const returnDepth = MAX_SETBACK + 0.6;
  for (let s = 0; s < f.stories; s++) {
    const y = y0 + s * STORY_H;
    const upperImg = f.kind === "cottage" ? tex.frame_clay
      : (timberUpper ? tex[f.upper] : tex[f.ground]);
    const img = s === 0 ? tex[f.ground] : upperImg;

    if (s > 0 && timberUpper) {
      const story = f.kind === "cottage" ? { ...f, upper: "frame_clay" } : f;
      timberStory(quads, story, tex, y, length, normal, xFront);
    } else {
      tiledWall(quads, img, [x, y, f.z0], [0, 0, 1], [0, 1, 0],
                length, STORY_H, WALL_TILE, STORY_H, normal, WALL_SHADE);
    }
    tiledWall(quads, img, [x, y, f.z1], [-street, 0, 0], [0, 1, 0],
              returnDepth, STORY_H, WALL_TILE, STORY_H, [0, 0, 1], WALL_SHADE);
    tiledWall(quads, img, [x, y, f.z0], [-street, 0, 0], [0, 1, 0],
              returnDepth, STORY_H, WALL_TILE, STORY_H, [0, 0, -1], WALL_SHADE);
  }

  const cornerFrac = CORNER_W / WALL_TILE;
  const cornerStories = timberUpper ? 1 : f.stories;
  const corner = (img, zEdge, dir, uvX0, uvX1) => {
    for (let s = 0; s < cornerStories; s++) {
      const y = y0 + s * STORY_H;
      quads.push({
        img, normal, shade: WALL_SHADE, layer: 1,
        pts: [[xFront(0.012), y, zEdge],
              [xFront(0.012), y, zEdge + dir * CORNER_W],
              [xFront(0.012), y + STORY_H, zEdge + dir * CORNER_W],
              [xFront(0.012), y + STORY_H, zEdge]],
        uvs: [[uvX0, img.height], [uvX1, img.height], [uvX1, 0], [uvX0, 0]],
      });
    }
  };
  const cl = tex[f.corner[0]];
  const cr = tex[f.corner[1]];
  corner(cl, f.z0, 1, 0, cl.width * cornerFrac);
  corner(cr, f.z1, -1, cr.width, cr.width * (1 - cornerFrac));

  if (f.stories > 1 && timberUpper) {
    const beam = tex.wall_timber;
    const beamY = y0 + STORY_H - 0.06;
    for (let u = 0; u < length - 1e-6; u += 1.4) {
      const uw = Math.min(1.4, length - u);
      const uPix = (uw / 1.4) * beam.width;
      quads.push({
        img: beam, normal, shade: WALL_SHADE, layer: 1, flat: true,
        pts: [[xFront(0.02), beamY, f.z0 + u],
              [xFront(0.02), beamY, f.z0 + u + uw],
              [xFront(0.02), beamY + 0.12, f.z0 + u + uw],
              [xFront(0.02), beamY + 0.12, f.z0 + u]],
        uvs: [[0, 12], [uPix, 12], [uPix, 0], [0, 0]],
      });
    }
  }

  if (f.kind === "terrace") {
    const pTop = wallTop + 0.3;
    tiledWall(quads, tex[f.ground], [x, wallTop, f.z0], [0, 0, 1], [0, 1, 0],
              length, 0.3, WALL_TILE, STORY_H, normal, WALL_SHADE);
    tiledWall(quads, tex.slab, [xFront(0.06), pTop, f.z0],
              [0, 0, 1], [-street, 0, 0], length, 0.36, 1.4, 1.4,
              [0, 1, 0], 0.5);
  } else {
    const eaveY = wallTop - 0.04;
    const xEave = xFront(0.18);
    const roofImg = f.roof === "thatch" ? tex.thatch : tex[f.roof];
    const slopeLen = Math.hypot(ROOF_RUN, ROOF_RISE);
    tiledWall(quads, roofImg,
              [xEave, eaveY, f.z0], [0, 0, 1],
              normOf([-street * ROOF_RUN, ROOF_RISE, 0]),
              length, slopeLen, ROOF_TILE, ROOF_TILE,
              normOf([street * ROOF_RISE, ROOF_RUN, 0]), 0.45);
    if (f.roof === "thatch") {
      for (let u = 0; u < length - 1e-6; u += 1.4) {
        const uw = Math.min(1.4, length - u);
        const uPix = (uw / 1.4) * 64;
        quads.push({
          img: tex.thatch_edge, normal, shade: 0.45, layer: 1, flat: true,
          pts: [[xEave, eaveY - 0.14, f.z0 + u], [xEave, eaveY - 0.14, f.z0 + u + uw],
                [xEave, eaveY + 0.1, f.z0 + u + uw], [xEave, eaveY + 0.1, f.z0 + u]],
          uvs: [[0, 64], [uPix, 64], [uPix, 40], [0, 40]],
        });
      }
    }
    const zMid = (f.z0 + f.z1) / 2;
    if (length > 4.3) {
      dormerQuads(quads, f, tex, f.z0 + length * 0.28, eaveY, xEave, street);
      dormerQuads(quads, f, tex, f.z0 + length * 0.72, eaveY, xEave, street);
    } else {
      dormerQuads(quads, f, tex, zMid, eaveY, xEave, street);
    }
  }

  for (const b of crateBoxes(f)) {
    const img = [tex.crate_planks, tex.crate_boarded, tex.crate][b.kind];
    quads.push(...boxFaces(b.x0, b.x1, b.y0, b.y0 + b.size,
                           b.zc - b.size / 2, b.zc + b.size / 2)
      .map((q) => ({ ...q, img, shade: 0.5, layer: 1 })));
  }

  const decal = (img, zc, w, yBase, yTop) => {
    const n = 2;
    const x2 = xFront(0.025);
    for (let i = 0; i < n; i++) {
      for (let j = 0; j < n; j++) {
        const za = zc - w / 2 + (w * i) / n;
        const zb = za + w / n;
        const ya = yBase + ((yTop - yBase) * j) / n;
        const yb = ya + (yTop - yBase) / n;
        const ua = (img.width * i) / n;
        const ub = (img.width * (i + 1)) / n;
        const va = img.height * (1 - j / n);
        const vb = img.height * (1 - (j + 1) / n);
        quads.push({
          img, normal, shade: WALL_SHADE, layer: 2,
          pts: [[x2, ya, za], [x2, ya, zb], [x2, yb, zb], [x2, yb, za]],
          uvs: [[ua, va], [ub, va], [ub, vb], [ua, vb]],
        });
      }
    }
  };

  for (let i = 0; i < SLOTS; i++) {
    const z = slotZ(f, i);
    const lit = f.lit[i];

    if (i === f.doorSlot) {
      const doorImg = f.doorMetal ? tex.door_metal
        : (lit || f.doorLit ? tex.door_lit : tex.door_shut);
      decal(doorImg, z, DOOR_W, y0, y0 + DOOR_H);
      const sOut = xFront(0.4);
      quads.push(...boxFaces(Math.min(x, sOut), Math.max(x, sOut),
                             y0, y0 + 0.07, z - 0.42, z + 0.42)
        .map((q) => ({ ...q, img: tex.slab, shade: 0.5 })));
    } else {
      decal(lit ? tex.window_lit : tex.window, z, WIN, y0 + 0.5, y0 + 0.5 + WIN);
    }

    if (f.stories > 1 && !timberUpper) {
      const yb = y0 + STORY_H + 0.42;
      decal(f.lit[(i + 1) % SLOTS] ? tex.window_lit : tex.window, z, WIN, yb, yb + WIN);
    }
  }

  return quads;
}

export function facadeShadow(ctx, cam, f) {
  const street = -f.side;
  const wallH = f.stories * STORY_H;
  const cast = f.side > 0;
  const len = cast ? 0.55 + wallH * 0.42 : 0.42;
  const alpha = cast ? 0.58 : 0.4;
  const zSkew = cast ? -0.22 * len : 0;
  const y = KERB_H + 0.008;
  const pts = [
    [f.xWall, y, f.z0], [f.xWall, y, f.z1],
    [f.xWall + street * len, y, f.z1 + zSkew],
    [f.xWall + street * len, y, f.z0 + zSkew],
  ];
  const proj = pts.map((p) => cam.project(p));
  if (proj.some((p) => !p)) return;
  const fogAmount = fogT((proj[0].z + proj[2].z) / 2);
  if (fogAmount >= HIDDEN) return;
  const a = alpha * (1 - fogAmount);
  const g = ctx.createLinearGradient(
    (proj[0].x + proj[1].x) / 2, (proj[0].y + proj[1].y) / 2,
    (proj[2].x + proj[3].x) / 2, (proj[2].y + proj[3].y) / 2);
  g.addColorStop(0, `rgba(0,0,0,${a})`);
  g.addColorStop(1, "rgba(0,0,0,0)");
  tracePath(ctx, proj);
  ctx.fillStyle = g;
  ctx.fill();
}
