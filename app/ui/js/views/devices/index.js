import { api } from "../../core/api.js";
import { $, el, renderList } from "../../core/dom.js";
import { ago } from "../../core/format.js";
import { connectionCard } from "./connection.js";
import { flashSection } from "./flash.js";

function render({ conn, devices, flash, brands, manifest }) {
  connectionCard(conn);
  flashSection({ flash, brands, manifest });

  renderList($("devices"), $("devices-empty"), devices, (d) => {
    const row = el("li");
    row.append(el("span", "dot ok"));
    row.append(el("span", "grow", d.device_id));
    row.append(el("span", "dim", ago(d.last_seen)));
    return row;
  });
}

export default {
  id: "devices",
  title: "Devices",
  load: async (status) => {
    const [conn, flash, brands, manifest] = await Promise.all([
      api("GET", "/api/connection"), api("GET", "/api/flash"),
      api("GET", "/api/brands"), api("GET", "/api/config"),
    ]);
    return { conn, flash, brands, manifest, devices: Object.values(status.devices || {}) };
  },
  render,
};
