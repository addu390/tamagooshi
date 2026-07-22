import { initChrome } from "../../common/js/components/chrome.js";
import { initKeyHints } from "../../common/js/components/keyhints.js";
import { initAccentPicker } from "../../common/js/components/accents.js";
import { initSetupTabs } from "../../common/js/components/tabs.js";
import { el } from "../../common/js/core/dom.js";
import { loadPack, frameSrc, defaultSelection } from "./pack.js";
import { createRenderer } from "./render.js";
import { renderFrame, renderSheet, downloadCanvas, mascotYaml, mascotTree, mascotId } from "./exporter.js";

initChrome();
initKeyHints();
initAccentPicker();

const PREVIEW_GRID = 96;
const PART_GRID = 20;

function partThumb(pack, renderer, slot, part) {
  const img = renderer.partImage(frameSrc(part, pack.frames[0]));
  const k = pack.pixel ? 1 : PART_GRID / Math.max(img.width, img.height);
  const classes = ["mx-pixel", slot.flip && "mx-flipped"].filter(Boolean);
  const canvas = el("canvas", { class: classes.join(" "), "aria-label": part.id,
                                width: Math.max(1, Math.round(img.width * k)),
                                height: Math.max(1, Math.round(img.height * k)) });
  const ctx = canvas.getContext("2d");
  ctx.imageSmoothingEnabled = !pack.pixel;
  ctx.imageSmoothingQuality = "high";
  ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
  return canvas;
}

function row(label, buttons) {
  return el("div", { class: "mx-slot" }, [
    el("span", { class: "mx-label", text: label }),
    el("div", { class: "mx-row" }, buttons),
  ]);
}

function slotRow(pack, renderer, slot, selection, onChange) {
  const buttons = [];

  const choose = (id) => { selection.parts[slot.id] = id; onChange(); };
  const sync = () => buttons.forEach((b) =>
    b.classList.toggle("on", (b.dataset.part || null) === (selection.parts[slot.id] || null)));

  if (slot.optional) {
    const none = el("button", { class: "mx-part mx-none", type: "button", text: "none" });
    none.addEventListener("click", () => choose(null));
    buttons.push(none);
  }

  slot.parts.forEach((part) => {
    const btn = el("button", { class: "mx-part", type: "button", "data-part": part.id },
                   [partThumb(pack, renderer, slot, part)]);
    btn.addEventListener("click", () => choose(part.id));
    buttons.push(btn);
  });

  return { node: row(slot.name, buttons), sync };
}

function paletteRow(pal, selection, onChange) {
  const buttons = pal.colors.map((color) => {
    const btn = el("button", { class: "swatch", type: "button", style: `background:${color}`,
                               "aria-label": `${pal.name} ${color}` });
    btn.addEventListener("click", () => { selection.colors[pal.id] = color; onChange(); });
    return btn;
  });

  const sync = () => buttons.forEach((b, i) =>
    b.classList.toggle("on", pal.colors[i] === selection.colors[pal.id]));

  return { node: row(pal.name, buttons), sync };
}

function outlineRow(selection, onChange) {
  const buttons = [[false, "none"], [true, "ink"]].map(([value, label]) => {
    const btn = el("button", { class: "mx-part mx-none", type: "button", text: label });
    btn.addEventListener("click", () => { selection.outline = value; onChange(); });
    btn.dataset.outline = value;
    return btn;
  });

  const sync = () => buttons.forEach((b) =>
    b.classList.toggle("on", b.dataset.outline === String(Boolean(selection.outline))));

  return { node: row("Outline", buttons), sync };
}

function frameGallery(pack, size, paintFrame, getFrame, setFrame) {
  const thumbs = pack.frames.map((frame) => {
    const canvas = el("canvas", { class: "mx-pixel", width: size.w, height: size.h });
    const btn = el("button", { class: "mx-thumb", type: "button", "aria-label": frame }, [canvas]);
    btn.addEventListener("click", () => setFrame(frame));
    return { frame, btn, ctx: canvas.getContext("2d") };
  });

  const sync = () => thumbs.forEach(({ frame, btn, ctx }) => {
    btn.classList.toggle("on", frame === getFrame());
    paintFrame(ctx, frame);
  });

  return { node: el("div", { class: "mx-thumbs" }, thumbs.map((t) => t.btn)), sync };
}

async function mountMixer(root, pack) {
  const renderer = await createRenderer(pack);

  const selection = defaultSelection(pack);
  let frame = pack.frames[0];

  const viewSize = pack.pixel ? pack.canvas : { w: PREVIEW_GRID, h: PREVIEW_GRID };
  const grid = pack.pixel ? null : el("canvas", { width: viewSize.w, height: viewSize.h });
  const paintFrame = (ctx2, f) => {
    if (pack.pixel) {
      renderer.drawFrame(ctx2, selection, f, ctx2.canvas.width / pack.canvas.w);
      return;
    }
    renderer.drawFrame(grid.getContext("2d"), selection, f, viewSize.w / pack.canvas.w);
    ctx2.imageSmoothingEnabled = false;
    ctx2.clearRect(0, 0, ctx2.canvas.width, ctx2.canvas.height);
    ctx2.drawImage(grid, 0, 0, ctx2.canvas.width, ctx2.canvas.height);
  };

  const displayPx = 200 * Math.min(devicePixelRatio || 1, 2);
  const previewScale = Math.max(1, Math.ceil(displayPx / viewSize.w));
  const canvas = el("canvas", { class: "mx-canvas mx-pixel",
                                width: viewSize.w * previewScale,
                                height: viewSize.h * previewScale });
  const ctx = canvas.getContext("2d");

  const nameInput = el("input", { class: "mx-name", type: "text", value: `My ${pack.id}`,
                                  "aria-label": `${pack.name} name` });
  const yaml = el("pre", { "data-lang": "config.yaml" });
  const tree = el("pre", { "data-lang": "directory" });

  const frameTag = el("span", { class: "mx-frametag", text: frame });

  const synced = [];
  const update = () => {
    synced.forEach((s) => s.sync());
    paintFrame(ctx, frame);
    frameTag.textContent = frame;
    yaml.textContent = mascotYaml(nameInput.value);
    tree.textContent = mascotTree(nameInput.value);
  };

  const controls = el("div", { class: "mx-controls" });
  const addControl = (r) => {
    synced.push(r);
    controls.appendChild(r.node);
  };
  const hasChoice = (slot) => slot.parts.length > 1 || slot.optional;
  pack.slots.filter(hasChoice).forEach((slot) =>
    addControl(slotRow(pack, renderer, slot, selection, update)));
  (pack.palettes || []).forEach((pal) => addControl(paletteRow(pal, selection, update)));
  if (pack.outline) addControl(outlineRow(selection, update));
  controls.appendChild(row("Name", [nameInput]));

  const saveStrip = el("button", { class: "btn", type: "button", text: "Download 6-frame" });
  saveStrip.addEventListener("click", () =>
    downloadCanvas(renderSheet(renderer, pack, selection), `${mascotId(nameInput.value)}.png`));

  const savePng = el("button", { class: "btn ghost", type: "button", text: "PNG" });
  savePng.addEventListener("click", () =>
    downloadCanvas(renderFrame(renderer, pack, selection, frame),
                   `${mascotId(nameInput.value)}-${frame}.png`));

  const stage = el("div", { class: "mx-stage" },
                   [el("div", { class: "mx-view" }, [frameTag, canvas])]);
  const gallery = frameGallery(pack, viewSize, paintFrame, () => frame,
                               (f) => { frame = f; update(); });
  synced.push(gallery);
  stage.appendChild(gallery.node);
  stage.appendChild(el("div", { class: "mx-actions" },
                       [el("div", { class: "mx-split" }, [saveStrip, savePng])]));
  stage.appendChild(el("small", { class: "mx-credit", text: pack.credit }));

  nameInput.addEventListener("input", update);
  root.append(el("div", { class: "mx-stagecol" }, [stage]), controls);
  root.appendChild(el("div", { class: "mx-yaml" }, [yaml, tree]));
  update();
}

async function initMixers() {
  const base = "mixer/assets/manifests/";
  const files = await (await fetch(base + "index.json")).json();
  const packs = await Promise.all(files.map((f) => loadPack(base + f)));

  const box = document.getElementById("mixer-root");
  const tabbar = box.querySelector(".tabbar");

  packs.forEach((pack, i) => {
    tabbar.appendChild(el("button", { class: i ? "tab" : "tab active", type: "button",
                                      "data-setup": pack.id, "aria-selected": String(!i),
                                      text: pack.name }));

    const mount = el("div", { class: "mixer" });
    const pane = el("div", { class: "setup-pane", "data-pane": pack.id }, [mount]);
    pane.hidden = i > 0;
    box.appendChild(pane);
    mountMixer(mount, pack);
  });

  initSetupTabs();
}

initMixers();
