import { Controller } from "../../../lib/controller.js";
import { fitCanvas, startLoop } from "../../../lib/loop.js";
import { World } from "./world.js";
import { render } from "./render.js";

const canvas = document.getElementById("view");
const ctx = canvas.getContext("2d");
const headlineEl = document.getElementById("headline");
const readoutEl = document.getElementById("readout");
const statusEl = document.getElementById("status");

const controller = new Controller();
const world = new World();

fitCanvas(canvas, ctx);

const fmt = (v) => `${v < 0 ? "-" : "+"}${Math.abs(v).toFixed(2)}`;

startLoop((dt, time) => {
  const frame = controller.poll(dt);
  world.step(frame, dt);
  render(ctx, canvas.clientWidth, canvas.clientHeight, world, time);

  headlineEl.textContent =
    `${world.rolls} ROLLS \u00b7 CELL ${world.col},${-world.row}`;
  readoutEl.textContent =
    `X ${fmt(frame.x)} Y ${fmt(frame.y)} \u00b7 A ${frame.a ? 1 : 0} B ${frame.b ? 1 : 0}`;
  statusEl.textContent = frame.connected ? "CONTROLLER LIVE" : "PAIR TO PLAY \u00b7 ARROWS TO TEST";
  statusEl.classList.toggle("live", frame.connected);
});
