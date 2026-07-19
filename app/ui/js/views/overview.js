import { $, el, renderList } from "../core/dom.js";
import { TREND_ARROW, severityClass } from "../core/format.js";

function fact(value, label) {
  const node = el("span");
  node.append(el("b", "", String(value)), " " + label);
  return node;
}

function render(status) {
  const metrics = status.metrics || [];
  const alerts = status.alerts || [];
  const devices = Object.values(status.devices || {});

  $("hero-mood").textContent = status.mood || "—";
  $("hero-facts").replaceChildren(
    fact(metrics.length, "metrics"),
    fact(alerts.length, "active alerts"),
    fact(devices.length, "devices"),
  );

  renderList($("metrics"), $("metrics-empty"), metrics, (m) => {
    const card = el("div", "card");
    card.append(el("div", "k", m.label || m.key));
    card.append(el("div", "v", String(m.value)));
    card.append(el("div", "t", TREND_ARROW[m.trend] || ""));
    return card;
  });

  renderList($("alerts"), $("alerts-empty"), alerts, (a) => {
    const row = el("li");
    row.append(el("span", "dot " + severityClass(a.severity)));
    row.append(el("span", "grow", a.title || a.id));
    if (a.acked) row.append(el("span", "badge", "acked"));
    row.append(el("span", "dim", a.severity || ""));
    return row;
  });
}

export default { id: "overview", title: "Overview", load: (status) => status, render };
