import { api } from "../../core/api.js";
import { $, el, renderList } from "../../core/dom.js";
import { ago } from "../../core/format.js";
import { connectionCard } from "./connection.js";

function render({ conn, devices }) {
  connectionCard(conn);

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
  load: async (status) => ({
    conn: await api("GET", "/api/connection"),
    devices: Object.values(status.devices || {}),
  }),
  render,
};
