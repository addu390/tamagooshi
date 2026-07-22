import { api, mutate } from "../../core/api.js";
import { $, el, showResult } from "../../core/dom.js";
import { formActions, panelHead } from "../../components/rows.js";

const LINKS = { ble: "Bluetooth", wifi: "Wi-Fi (MQTT)" };

const STATES = {
  idle: { dot: "", text: () => "Not connected. Scan to pick a device." },
  pairing_declined: {
    dot: "crit",
    text: (name) => `Pairing with ${name || "the device"} was declined. Connect again to retry.`,
  },
  connected: { dot: "ok", text: (name) => (name ? `Connected to ${name}.` : "Connected.") },
  connecting: { dot: "warn", text: (name) => `Connecting to ${name || "device"}…` },
  reconnecting: { dot: "warn", text: () => "Link lost, reconnecting…" },
  scanning: { dot: "warn", text: (name) => `Scanning for ${name || "a device"} nearby…` },
};

function connect(device) {
  $("scan-panel").classList.add("hide");
  mutate(() => api("PUT", "/api/connection", { transport: "ble:gatt", device }), "devices-note");
}

function scanRow(device) {
  const row = el("li");
  const name = el("span", "", device.name);
  name.title = device.address;
  row.append(name);
  row.append(el("span", "grow dim", device.address));
  row.append(el("span", "dim", `${device.rssi} dBm`));

  const pick = el("button", "btn primary", "Connect");
  pick.addEventListener("click", () => connect({ name: device.name, address: device.address }));
  row.append(pick);
  return row;
}

async function runScan(list, note) {
  showResult(note, "Scanning nearby…");
  list.replaceChildren();

  try {
    const { devices } = await api("POST", "/api/connection/scan");
    if (!devices.length) {
      showResult(note, "No devices found. Make sure one is powered on nearby.", true);
      return;
    }

    note.classList.add("hide");
    list.replaceChildren(...devices.map(scanRow));
  } catch (err) {
    showResult(note, err.message, true);
  }
}

function openScan() {
  const panel = $("scan-panel");
  panel.classList.remove("hide");
  panel.replaceChildren();

  panel.append(panelHead("Nearby devices",
                         "Pick the device this hub should feed. Connecting restarts the hub."));

  const list = el("ul", "rows");
  const note = el("p", "form-result hide");
  panel.append(list, note);

  const cancel = el("button", "btn", "Cancel");
  cancel.addEventListener("click", () => panel.classList.add("hide"));

  const again = el("button", "btn", "Scan again");
  again.addEventListener("click", () => runScan(list, note));

  panel.append(formActions([cancel, again]));

  runScan(list, note);
}

export function connectionCard(conn) {
  const card = $("connection");
  card.replaceChildren();

  const state = STATES[conn.state] || { dot: "", text: () => conn.state };
  const name = conn.device?.name || conn.device?.address;

  const row = el("div", "set-row");
  const info = el("div", "set-info");
  const title = el("div", "set-title");
  title.append(el("span", "dot " + state.dot), ` ${LINKS[conn.link] || conn.link}`);
  info.append(title, el("div", "set-desc", state.text(name)));
  row.append(info);

  const buttons = el("div", "chips");
  if (conn.locked) {
    info.append(el("div", "set-desc", "Transport fixed by TAMA_TRANSPORT on the host."));
  }

  if (conn.saved) {
    const forget = el("button", "btn", "Forget device");
    forget.addEventListener("click", () =>
      mutate(() => api("DELETE", "/api/connection/device"), "devices-note"));
    buttons.append(forget);
  }

  if (conn.link === "ble") {
    const scan = el("button", "btn", "Scan for devices");
    scan.addEventListener("click", openScan);
    buttons.append(scan);
  } else if (!conn.locked) {
    const scan = el("button", "btn", "Connect via Bluetooth");
    scan.addEventListener("click", openScan);
    buttons.append(scan);
  }

  row.append(buttons);
  card.append(row);
}
