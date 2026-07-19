import { api, awaitRestart, mutate } from "../core/api.js";
import { $, el, renderList, showResult } from "../core/dom.js";
import { condition, opSymbol, severityClass } from "../core/format.js";
import { editHold } from "../core/hold.js";
import { fieldInput, labeledControl, segControl, selectField, switchControl }
  from "../components/controls.js";
import { formActions } from "../components/rows.js";
import { readFields } from "../components/schema.js";

const guard = editHold();
let state = { moods: [], alerts: [], metricKeys: [],
              options: { moods: [], severities: [], ops: [] } };

function metricField(value) {
  const input = fieldInput("metric", {}, value);
  input.setAttribute("list", "metric-keys");
  return input;
}

function conditionFields(root, when) {
  root.append(labeledControl("metric", metricField(when?.metric)));

  let op = when?.op || "gt";
  root.append(labeledControl("condition",
                             segControl(state.options.ops, op, opSymbol, (o) => { op = o; })));
  root.append(labeledControl("value", fieldInput("value", { type: "number" }, when?.value)));

  return () => {
    const flat = readFields(root);
    return { metric: flat.metric || "", op, value: flat.value ?? 0 };
  };
}

function moodFields(root, rule) {
  const readWhen = conditionFields(root, rule?.when);

  const moods = state.options.moods.map((m) => [m, m]);
  root.append(labeledControl("mood", selectField("mood", moods, rule?.mood || moods[0]?.[0])));
  root.append(labeledControl("priority",
                             fieldInput("priority", { type: "integer", default: 0 },
                                        rule?.priority)));

  return () => {
    const flat = readFields(root);
    return { when: readWhen(), mood: flat.mood, priority: flat.priority ?? 0 };
  };
}

function alertFields(root, rule) {
  root.append(labeledControl("id", fieldInput("id", {}, rule?.id)));
  root.append(labeledControl("title", fieldInput("title", {}, rule?.title)));
  root.append(labeledControl("body", fieldInput("body", {}, rule?.body)));

  const readWhen = conditionFields(root, rule?.when);

  let severity = rule?.severity || "warning";
  root.append(labeledControl("severity",
                             segControl(state.options.severities, severity, (s) => s,
                                        (s) => { severity = s; })));

  let ack = rule?.requires_ack ?? true;
  const sw = switchControl(ack, () => {
    ack = !ack;
    sw.classList.toggle("on", ack);
  });
  root.append(labeledControl("requires ack", sw));

  return () => {
    const flat = readFields(root);
    return { id: flat.id || "", title: flat.title || "", body: flat.body || "",
             when: readWhen(), severity, requires_ack: ack,
             source: rule?.source || "hub" };
  };
}

function editor(buildFields, rule, save) {
  guard.hold();

  const row = el("li", "edit-row");
  const box = el("div", "qedit");
  const fields = el("div", "form-fields");
  const read = buildFields(fields, rule);
  box.append(fields);

  const result = el("p", "form-result hide");

  const cancel = el("button", "btn", "Cancel");
  cancel.addEventListener("click", () => {
    guard.release();
    renderAll();
  });

  const saveBtn = el("button", "btn primary", "Save");
  saveBtn.addEventListener("click", async () => {
    try {
      await save(read());
      awaitRestart("rules-note");
    } catch (err) {
      showResult(result, err.message, true);
    }
  });

  box.append(result, formActions([cancel, saveBtn]));
  row.append(box);
  return row;
}

function putList(kind, list) {
  return api("PUT", `/api/rules/${kind}`, list);
}

function replaceAt(list, index, rule) {
  if (index == null) return [...list, rule];
  return list.map((r, i) => (i === index ? rule : r));
}

function ruleActions(row, kind, buildFields, index) {
  const list = state[kind];

  const edit = el("button", "btn", "Edit");
  edit.addEventListener("click", () => {
    row.replaceWith(editor(buildFields, list[index],
                           (rule) => putList(kind, replaceAt(list, index, rule))));
  });

  const drop = el("button", "btn", "Delete");
  drop.addEventListener("click", () => {
    if (!confirm("Delete this rule?")) return;
    mutate(() => putList(kind, list.filter((_, i) => i !== index)), "rules-note");
  });

  return [edit, drop];
}

function moodRow(r, index) {
  const row = el("li");
  row.append(el("span", "dot"));
  row.append(el("span", "grow", `${condition(r.when)}  →  ${r.mood}`));
  row.append(el("span", "dim", "priority " + r.priority));
  row.append(...ruleActions(row, "moods", moodFields, index));
  return row;
}

function alertRow(r, index) {
  const row = el("li");
  row.append(el("span", "dot " + severityClass(r.severity)));
  row.append(el("span", "grow", `${r.title} · ${condition(r.when)}`));
  row.append(el("span", "dim", r.severity));
  row.append(...ruleActions(row, "alerts", alertFields, index));
  return row;
}

function renderAll() {
  renderList($("moods"), $("moods-empty"), state.moods || [], moodRow);
  renderList($("rule-alerts"), $("rule-alerts-empty"), state.alerts || [], alertRow);

  $("metric-keys").replaceChildren(...state.metricKeys.map((key) => {
    const opt = el("option");
    opt.value = key;
    return opt;
  }));
}

function render(data) {
  if (guard.isHeld()) return;
  state = data;
  renderAll();
}

async function load() {
  const [rules, sources] = await Promise.all([api("GET", "/api/rules"),
                                              api("GET", "/api/sources")]);
  const metricKeys = [...new Set(sources.flatMap((s) => s.metrics.map((m) => m.key)))];
  return { ...rules, metricKeys };
}

function init() {
  $("add-mood").addEventListener("click", () => {
    $("moods").append(editor(moodFields, null,
                             (rule) => putList("moods", [...state.moods, rule])));
  });

  $("add-alert").addEventListener("click", () => {
    $("rule-alerts").append(editor(alertFields, null,
                                   (rule) => putList("alerts", [...state.alerts, rule])));
  });
}

export default { id: "rules", title: "Rules", init, load, render };
