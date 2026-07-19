import { api } from "./core/api.js";
import { $ } from "./core/dom.js";
import { applyPrefs, darkQuery } from "./core/prefs.js";
import { activeView, initRouter } from "./core/router.js";
import views, { NAV } from "./views/index.js";

function renderChrome(status) {
  const name = status.brand?.name || "TAMAGOOSHI";
  $("brand-name").textContent = name;
  $("brand-logo").textContent = name.charAt(0).toUpperCase();
  $("brand-tagline").textContent = status.brand?.tagline || "";

  const mood = $("mood-chip");
  mood.textContent = "mood: " + (status.mood || "—");
  mood.className = "chip" + (status.mood ? " on" : "");

  const devices = Object.values(status.devices || {});
  const device = $("device-chip");
  device.textContent = devices.length
    ? devices.map((d) => d.device_id).join(", ")
    : "no device";
  device.className = "chip" + (devices.length ? " info" : "");

  $("hub-line").textContent = "hub · " + (status.device_id || "");
  $("hub-dot").className = "dot ok";
}

async function refresh() {
  try {
    const status = await api("GET", "/api/status");
    renderChrome(status);

    const view = activeView();
    view.render(await view.load(status));
  } catch {
    $("device-chip").textContent = "hub unreachable";
    $("device-chip").className = "chip crit";
    $("hub-dot").className = "dot crit";
  }
}

darkQuery.addEventListener("change", applyPrefs);
applyPrefs();

for (const view of views) view.init?.();
initRouter(NAV, refresh);

const events = new EventSource("/api/events");
for (const type of ["mood", "metric", "alert.raised", "alert.cleared", "device", "link"]) {
  events.addEventListener(type, refresh);
}
setInterval(refresh, 15000);
