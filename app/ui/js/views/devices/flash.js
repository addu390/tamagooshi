import catalog from "../../catalog.gen.js";
import { api } from "../../core/api.js";
import { $, el, showResult } from "../../core/dom.js";
import { editHold } from "../../core/hold.js";
import { selectField } from "../../components/controls.js";
import { formActions, settingRow } from "../../components/rows.js";
import { encode, fromManifest } from "../../wire/blob.js";
import { EWT_URL, configPart, imageName, imageUrl, installButton, installerManifest,
         revokeUrls } from "../../wire/flasher.js";

const hold = editHold();
const manifests = new Map();

let boardId = catalog.boards[0].id;
let brandId = null;
let last = null;

const board = () => catalog.boards.find((b) => b.id === boardId) || catalog.boards[0];
const configBlob = (manifest) => encode(fromManifest(manifest));

function boardRow() {
  const select = selectField("board", catalog.boards.map((b) => [b.id, b.name]), boardId,
                             { ctl: true });
  select.addEventListener("change", () => {
    boardId = select.value;
    flashSection(last);
  });
  return settingRow("Board", "Model of the stick plugged in over USB.", select);
}

function brandRow(brands) {
  const sorted = [...brands].sort(
    (a, b) => (b.active ? 1 : 0) - (a.active ? 1 : 0) || a.name.localeCompare(b.name));
  const dupes = new Set(sorted.filter(
    (b) => sorted.some((o) => o !== b && o.name === b.name)).map((b) => b.name));
  const options = sorted.map((b) => {
    const name = dupes.has(b.name) ? `${b.name} (${b.id})` : b.name;
    return [b.id, b.active ? `${name} · active` : name];
  });

  const select = selectField("brand", options, brandId, { ctl: true });
  select.addEventListener("change", async () => {
    brandId = select.value;
    if (!manifests.has(brandId)) {
      manifests.set(brandId, await api("GET", `/api/brands/${brandId}/manifest`));
    }
    flashSection(last);
  });
  return settingRow("Brand", "Configuration written alongside the firmware image.", select);
}

function progressBar() {
  const fill = el("span");
  const node = el("div", "flash-bar hide");
  node.append(fill);

  const set = (pct) => {
    node.classList.remove("hide");
    fill.style.width = Math.max(0, Math.min(100, pct)) + "%";
  };
  return { node, set };
}

let ewt = null;

function browserPath(root, manifest) {
  ewt = ewt || import(EWT_URL);
  revokeUrls();

  const image = { path: imageUrl(catalog.release, manifest, board()), offset: 0 };
  const branded = installButton("Flash brand", "btn primary",
                                installerManifest(board().chipFamily, [
                                  image, configPart(configBlob(manifest), board().configOffset),
                                ]));
  const stock = installButton("Flash defaults", "btn",
                              installerManifest(board().chipFamily, [image]));

  root.append(formActions([stock, branded]));
  root.append(el("p", "form-result", "Flashing runs in this browser over USB."));
}

function watchJob(job, bar, note) {
  const events = new EventSource("/api/events");
  events.addEventListener("flash", (e) => {
    const d = JSON.parse(e.data);
    if (d.job !== job) return;

    if (d.pct != null) bar.set(d.pct);
    if (d.message) showResult(note, d.message, d.state === "error");
    if (d.state === "finished" || d.state === "error") {
      events.close();
      hold.release();
    }
  });
}

async function startFlash(payload, bar, note) {
  hold.hold();
  showResult(note, "Starting…");
  bar.set(0);

  try {
    const { job } = await api("POST", "/api/flash", payload);
    watchJob(job, bar, note);
  } catch (err) {
    hold.release();
    showResult(note, err.message, true);
  }
}

function rescanButton() {
  const btn = el("button", "btn", "Rescan");
  btn.addEventListener("click", async () => {
    flashSection({ ...last, flash: await api("GET", "/api/flash") });
  });
  return btn;
}

function hubPath(root, status, manifest) {
  const ports = status.ports.map((p) => [p.device, p.description || p.device]);

  if (!ports.length) {
    root.append(settingRow("Port",
                           "No USB serial ports found. Plug the device in, then rescan.",
                           rescanButton()));
    return;
  }

  const select = selectField("port", ports, ports[0][0], { ctl: true });
  root.append(settingRow("Port", "Serial port the hub flashes through.", select));

  const bar = progressBar();
  const note = el("p", "form-result hide");

  const payload = (withConfig) => ({
    port: select.value,
    board: boardId,
    image_url: imageUrl(catalog.release, manifest, board()),
    ...(withConfig ? {
      config_offset: board().configOffset,
      config_b64: btoa(String.fromCharCode(...configBlob(manifest))),
    } : {}),
  });

  const stock = el("button", "btn", "Flash defaults");
  stock.addEventListener("click", () => startFlash(payload(false), bar, note));

  const branded = el("button", "btn primary", "Flash brand");
  branded.addEventListener("click", () => startFlash(payload(true), bar, note));

  if (status.busy) {
    stock.disabled = true;
    branded.disabled = true;
    showResult(note, "A flash is already running.");
  }

  root.append(formActions([stock, branded]), bar.node, note);
}

function unavailablePath(root) {
  const btn = el("button", "btn", "Flash device");
  btn.disabled = true;
  btn.title = "Not available in this view";

  root.append(formActions([btn]));
  root.append(el("p", "form-result",
                 "Flashing needs USB access: open this dashboard in Chrome or Edge, " +
                 "or run the hub on the machine the device is plugged into."));
}

export function flashSection(data) {
  if (hold.isHeld()) return;
  last = data;

  const root = $("flash");
  root.replaceChildren();

  const { flash: status, brands, manifest: activeManifest } = data;
  const active = brands.find((b) => b.active);
  if (active) manifests.set(active.id, activeManifest);
  if (!brandId || !brands.some((b) => b.id === brandId)) brandId = active?.id;

  const manifest = manifests.get(brandId) || activeManifest;
  root.append(boardRow(), brandRow(brands));
  root.append(settingRow("Image", "Latest release build for this board.",
                         el("span", "dim", imageName(manifest, board()))));

  if (status.available && (status.ports.length || !navigator.serial)) {
    hubPath(root, status, manifest);
  } else if (navigator.serial) {
    browserPath(root, manifest);
  } else {
    unavailablePath(root);
  }
}
