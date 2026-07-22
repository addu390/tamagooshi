import { Controller } from "../../../lib/controller.js";
import { fitCanvas, startLoop } from "../../../lib/loop.js";
import { World } from "./world.js";
import { render } from "./render.js";
import { loadTextures } from "./textures.js";

const canvas = document.getElementById("view");
const ctx = canvas.getContext("2d");
const headlineEl = document.getElementById("headline");
const readoutEl = document.getElementById("readout");
const statusEl = document.getElementById("status");

const controller = new Controller();
const world = new World();
const textures = await loadTextures();

fitCanvas(canvas, ctx);

const fmt = (v) => `${v < 0 ? "-" : "+"}${Math.abs(v).toFixed(2)}`;

startLoop((dt, time) => {
  const frame = controller.poll(dt);

  if (world.crashed && frame.aHit) world.reset();

  world.step(frame, dt);
  render(ctx, canvas.clientWidth, canvas.clientHeight, world, time, textures);

  headlineEl.textContent = world.crashed
    ? `CRASHED \u00b7 ${world.coins} COINS \u00b7 ${world.distance}M \u00b7 A TO RESTART`
    : `${world.coins} COINS \u00b7 ${world.distance}M`;
  headlineEl.classList.toggle("crashed", world.crashed);

  readoutEl.textContent = `X ${fmt(frame.x)} Y ${fmt(frame.y)} \u00b7 ${world.pace(frame).toUpperCase()}`;
  statusEl.textContent = frame.connected ? "LIVE" : "PAIR TO PLAY \u00b7 ARROWS TO TEST";
  statusEl.classList.toggle("live", frame.connected);
});
