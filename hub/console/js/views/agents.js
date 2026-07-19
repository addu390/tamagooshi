import { api, mutate } from "../core/api.js";
import { $, el, renderList } from "../core/dom.js";
import { switchControl } from "../components/controls.js";

function save(defaultId, enabled) {
  return mutate(() => api("PUT", "/api/agents", { default: defaultId, enabled }), "agents-note");
}

function toggle(agents, a) {
  const enabled = agents.filter((x) => (x.id === a.id ? !a.enabled : x.enabled))
    .map((x) => x.id);
  const current = agents.find((x) => x.default)?.id;
  const defaultId = enabled.includes(current) ? current : enabled[0] || current;
  save(defaultId, enabled);
}

function makeDefault(agents, a) {
  const enabled = agents.filter((x) => x.enabled || x.id === a.id).map((x) => x.id);
  save(a.id, enabled);
}

function render(agents) {
  renderList($("agents"), null, agents, (a) => {
    const row = el("li");
    row.append(el("span", "dot " + (a.cli && a.sdk ? "ok" : "crit")));
    row.append(el("span", "grow", a.label));

    row.append(el("span", "badge " + (a.cli ? "ok" : "miss"), a.cli ? "cli" : "cli missing"));
    row.append(el("span", "badge " + (a.sdk ? "ok" : "miss"), a.sdk ? "sdk" : "sdk missing"));

    if (a.default) {
      row.append(el("span", "badge ok", "default"));
    } else {
      const pick = el("button", "btn", "Make default");
      pick.addEventListener("click", () => makeDefault(agents, a));
      row.append(pick);
    }

    row.append(switchControl(a.enabled, () => toggle(agents, a)));
    return row;
  });
}

export default { id: "agents", title: "Agents", load: () => api("GET", "/api/agents"), render };
