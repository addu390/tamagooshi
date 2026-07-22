import { Camera, Follower, hash2 } from "../../../lib/math3d.js";
import { boxVerts, rollingVerts } from "../../core/roller.js";
import { C } from "./palette.js";
import { facades, SIDEWALK_W, MAX_SETBACK, KERB_H } from "./village.js";
import { facadeQuads, facadeShadow, crateBoxes } from "./buildings.js";
import { drawCrate, drawCoin } from "./props.js";
import {
  quadLit, fogT, HIDDEN, texQuad, drawQuads, drawTexQuadWorld, softShadow,
} from "./scene.js";

const FOV = 1.9;
const CAM_BACK = 6.4;
const CAM_UP = 4.6;
const CUBE = 0.92;
const AHEAD = 16;
const BEHIND = 3;

function drawFloor(ctx, cam, world, xOf, centerRow, tex) {
  const rowFar = Math.floor(centerRow - AHEAD);
  const rowNear = Math.ceil(centerRow + BEHIND);
  const topDark = 1 - quadLit([0, 1, 0]);
  const half = world.lanes / 2;

  for (let row = rowFar; row <= rowNear; row++) {
    const close = cam.depth([0, 0, row + 0.5]) < 6;

    for (let lane = 0; lane < world.lanes; lane++) {
      const h = hash2(row, lane);
      const img = h > 0.88 ? tex.slab : tex.pavement;
      texQuad(ctx, cam, img,
              [[xOf(lane), 0, row], [xOf(lane + 1), 0, row],
               [xOf(lane + 1), 0, row + 1], [xOf(lane), 0, row + 1]],
              { dark: topDark * 0.55 + (h - 0.5) * 0.1, subdivide: close ? 2 : 1 });
    }

    for (const side of [-1, 1]) {
      const xIn = side * half;
      const xOut = side * (half + SIDEWALK_W + MAX_SETBACK + 0.05);
      const h = hash2(row, side * 91);
      texQuad(ctx, cam, tex.slab,
              [[xIn, KERB_H, row], [xOut, KERB_H, row],
               [xOut, KERB_H, row + 1], [xIn, KERB_H, row + 1]],
              { dark: topDark * 0.5 + (h - 0.5) * 0.06 });
      if (h > 0.82) {
        const gIn = side * (half + 0.2);
        const gOut = side * (half + 1.1);
        texQuad(ctx, cam, tex.grate,
                [[gIn, KERB_H + 0.004, row + 0.05], [gOut, KERB_H + 0.004, row + 0.05],
                 [gOut, KERB_H + 0.004, row + 0.95], [gIn, KERB_H + 0.004, row + 0.95]],
                { dark: topDark * 0.5 });
      }

      drawTexQuadWorld(ctx, cam, tex.wall_stone,
                       side > 0
                         ? [[xIn, KERB_H, row], [xIn, KERB_H, row + 1],
                            [xIn, 0, row + 1], [xIn, 0, row]]
                         : [[xIn, KERB_H, row + 1], [xIn, KERB_H, row],
                            [xIn, 0, row], [xIn, 0, row + 1]],
                       [[0, 0], [64, 0], [64, 10], [0, 10]],
                       (1 - quadLit([-side, 0, 0])) * 0.5);
    }
  }
}

function playerVerts(world, xOf) {
  const inset = (1 - CUBE) / 2;
  return rollingVerts(xOf(world.lane) + inset, world.row + inset, CUBE, world.roll);
}

const follower = new Follower();

export function render(ctx, w, h, world, time, tex) {
  const sky = ctx.createLinearGradient(0, 0, 0, h);
  sky.addColorStop(0, C.skyTop);
  sky.addColorStop(0.42, C.skyHorizon);
  sky.addColorStop(1, C.skyLow);
  ctx.fillStyle = sky;
  ctx.fillRect(0, 0, w, h);

  const xOf = (lane) => lane - world.lanes / 2;
  const c = world.center();
  const eye = follower.update(c, time);
  const cam = new Camera(w, h, [xOf(eye.x), 0.45, eye.y], CAM_BACK, CAM_UP, FOV);

  drawFloor(ctx, cam, world, xOf, c.y, tex);

  const fs = [...facades(Math.floor(c.y - AHEAD), Math.ceil(c.y + BEHIND), world.lanes / 2)]
    .map((f) => ({ f, depth: cam.depth([f.xWall, 1, (f.z0 + f.z1) / 2]) }))
    .sort((a, b) => b.depth - a.depth);

  for (const { f } of fs) {
    facadeShadow(ctx, cam, f);
    for (const b of crateBoxes(f)) {
      if (b.y0 > KERB_H) continue;
      softShadow(ctx, cam, [(b.x0 + b.x1) / 2, KERB_H, b.zc], b.size,
                 b.size * 0.95, b.size * 0.7, 0.66);
    }
  }
  fs.forEach(({ f }) => drawQuads(ctx, cam, facadeQuads(f, tex)));

  softShadow(ctx, cam, [xOf(c.x), 0, c.y], CUBE, 0.8, 0.5, 0.75);

  const drawables = [{
    depth: cam.depth([xOf(c.x), 0.45, c.y]),
    draw: () => drawCrate(ctx, cam, tex.crate_player, playerVerts(world, xOf), world.crashed),
  }];

  for (const item of world.track.visible(Math.floor(c.y - AHEAD), Math.ceil(c.y + BEHIND))) {
    const laneX = xOf(item.lane + 0.5);
    if (item.type === "block") {
      const size = 0.8;
      const center = [laneX, size / 2, item.row + 0.5];
      softShadow(ctx, cam, [center[0], 0, center[2]], size, 0.62, 0.4, 0.7);
      drawables.push({
        depth: cam.depth(center),
        draw: () => drawCrate(ctx, cam, tex.crate,
                              boxVerts(laneX - size / 2, item.row + 0.5 - size / 2, size, size),
                              false),
      });
    } else {
      const bob = 0.44 + 0.06 * Math.sin(time * 3 + item.row);
      const center = [laneX, bob, item.row + 0.5];
      softShadow(ctx, cam, [center[0], 0, center[2]], 0,
                 0.28 - bob * 0.12, 0.17 - bob * 0.07, 0.62);
      drawables.push({
        depth: cam.depth(center),
        draw: () => drawCoin(ctx, cam, tex, center, time),
      });
    }
  }

  drawables.sort((a, b) => b.depth - a.depth).forEach((d) => d.draw());

  const vignette = ctx.createRadialGradient(w / 2, h * 0.55, h * 0.2, w / 2, h * 0.55, h * 0.95);
  vignette.addColorStop(0, "rgba(0,0,0,0)");
  vignette.addColorStop(1, "rgba(5,4,10,0.6)");
  ctx.fillStyle = vignette;
  ctx.fillRect(0, 0, w, h);
}
