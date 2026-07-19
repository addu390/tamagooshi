import { api, awaitRestart, mutate } from "../../core/api.js";
import { el, showResult } from "../../core/dom.js";
import { ago, plural } from "../../core/format.js";
import { formActions, secHead, settingRow, typeTile } from "../../components/rows.js";
import { fieldInput, labeledField, pollingControl, switchControl } from "../../components/controls.js";
import { connectionProps, readFields, resolveItems, scalarProps } from "../../components/schema.js";
import { expanded, holdEdits, reload } from "./state.js";

function sourceHealth(s) {
  if (s.error) return { cls: "err", text: s.error };

  const failing = s.metrics.filter((m) => m.error).length;
  if (failing) return { cls: "err", text: `${failing} of ${plural(s.metrics.length, "query")} failing` };

  const base = plural(s.metrics.length, "query");
  if (!s.enabled) return { cls: "", text: base + " · paused" };
  return { cls: "", text: `${base} · every ${s.interval_secs}s · last poll ${ago(s.last_emit)}` };
}

export function sourceCard(s, t) {
  const open = expanded.has(s.index);
  const card = el("div", "card source");

  const head = el("div", "src-head");
  const zone = el("button", "src-zone");
  zone.append(typeTile(s.type, s.label));

  const id = el("div", "src-id");
  const health = sourceHealth(s);
  id.append(el("div", "src-title", s.label || s.type));
  id.append(el("div", "src-sub" + (health.cls ? " " + health.cls : ""), health.text));
  zone.append(id, el("span", "chev" + (open ? " open" : ""), "›"));

  zone.addEventListener("click", () => {
    if (open) expanded.delete(s.index);
    else expanded.add(s.index);
    reload(true);
  });
  head.append(zone);

  const controls = el("div", "src-controls");
  const poll = el("button", "btn", "Poll now");
  poll.addEventListener("click", async () => {
    poll.textContent = "Polling…";
    poll.disabled = true;
    try { await api("POST", `/api/sources/${s.index}/refresh`); }
    catch (err) { alert(err.message); }
    reload(true);
  });

  const sw = switchControl(s.enabled, async () => {
    await api("POST", `/api/sources/${s.index}/toggle`, { enabled: !s.enabled });
    reload(true);
  });

  controls.append(poll, sw);
  head.append(controls);
  card.append(head);

  if (open && t) card.append(sourceBody(s, t));
  return card;
}

function sourceBody(s, t) {
  const body = el("div", "src-body");
  const props = t.schema.properties || {};
  const itemSchema = resolveItems(t.schema, props.metrics || {});

  const addQuery = el("button", "ghost", "+ Add query");
  body.append(secHead("Queries", addQuery));

  const qlist = el("div", "qlist");
  s.metrics.forEach((m, i) => qlist.append(queryRow(s, i, m, itemSchema)));
  if (!s.metrics.length) qlist.append(el("p", "empty", "No queries yet."));
  body.append(qlist);

  addQuery.addEventListener("click", () => {
    holdEdits();
    qlist.append(queryEditor(s, null, itemSchema));
    addQuery.disabled = true;
  });

  body.append(secHead("Settings"));

  const settings = el("div", "sec");
  settings.append(settingRow("Poll every", "How often queries run while the source is on.",
                             pollingControl(s.config)));
  for (const [name, prop] of connectionProps(t.schema)) {
    settings.append(settingRow(name.replaceAll("_", " "), null,
                               fieldInput(name, prop, s.config[name], { ctl: true })));
  }
  body.append(settings);

  if (s.secrets.length) {
    body.append(secHead("Credentials"));
    const creds = el("div", "sec");
    for (const secret of s.secrets) creds.append(secretRow(secret));
    body.append(creds);
  }

  const remove = el("button", "btn", "Remove source");
  remove.addEventListener("click", () => {
    if (!confirm(`Remove this ${s.label || s.type} source?`)) return;
    mutate(() => api("DELETE", `/api/sources/${s.index}`), "sources-note");
  });

  const save = el("button", "btn primary", "Save changes");
  save.addEventListener("click", () =>
    mutate(() => api("PUT", `/api/sources/${s.index}`, { ...s.config, ...readFields(settings) }),
           "sources-note"));

  body.append(formActions([save], { left: [remove], divided: true }));
  return body;
}

function queryRow(s, i, m, itemSchema) {
  const row = el("div", "qrow");
  row.append(el("span", "dot " + (m.error ? "crit" : m.value != null ? "ok" : "")));

  const name = el("div", "q-name");
  name.append(el("div", "q-key", m.key));
  if (m.query) name.append(el("div", "q-query", m.query));
  row.append(name);

  const state = el("div", "q-state" + (m.error ? " err" : ""),
                   m.error ? m.error : m.value != null ? String(m.value) : "no data");
  row.append(state);

  const actions = el("div", "q-actions");
  const test = el("button", "btn", "Test");
  test.addEventListener("click", async () => {
    state.className = "q-state";
    state.textContent = "testing…";

    try {
      const { metrics } = await api("POST", "/api/sources/test",
                                    { ...s.config, metrics: [s.config.metrics[i]] });
      state.textContent = metrics[0] ? String(metrics[0].value) : "no data";
    } catch (err) {
      state.className = "q-state err";
      state.textContent = err.message;
    }
  });

  const edit = el("button", "btn", "Edit");
  edit.addEventListener("click", () => {
    holdEdits();
    row.replaceWith(queryEditor(s, i, itemSchema));
  });

  const drop = el("button", "btn", "Remove");
  drop.addEventListener("click", () => {
    if (!confirm(`Remove query "${m.key}"?`)) return;
    const metrics = s.config.metrics.filter((_, j) => j !== i);
    mutate(() => api("PUT", `/api/sources/${s.index}`, { ...s.config, metrics }), "sources-note");
  });

  actions.append(test, edit, drop);
  row.append(actions);
  return row;
}

export function queryFields(itemSchema, values) {
  const wrap = el("div", "form-fields");
  for (const [name, prop] of scalarProps(itemSchema)) {
    wrap.append(labeledField(name, prop, values[name]));
  }
  return wrap;
}

function queryEditor(s, index, itemSchema) {
  const box = el("div", "qedit");
  const fields = queryFields(itemSchema, index == null ? {} : s.config.metrics[index]);
  box.append(fields);

  const result = el("p", "form-result hide");

  const cancel = el("button", "btn", "Cancel");
  cancel.addEventListener("click", () => reload(true));

  const test = el("button", "btn", "Test");
  test.addEventListener("click", async () => {
    showResult(result, "Testing…");
    try {
      const { metrics } = await api("POST", "/api/sources/test",
                                    { ...s.config, metrics: [readFields(fields)] });
      showResult(result, metrics[0] ? `${metrics[0].key} = ${metrics[0].value}` : "no data");
    } catch (err) { showResult(result, err.message, true); }
  });

  const save = el("button", "btn primary", "Save");
  save.addEventListener("click", async () => {
    const metric = readFields(fields);
    const metrics = [...s.config.metrics];
    if (index == null) metrics.push(metric);
    else metrics[index] = metric;

    try {
      await api("PUT", `/api/sources/${s.index}`, { ...s.config, metrics });
      awaitRestart("sources-note");
    } catch (err) { showResult(result, err.message, true); }
  });

  box.append(result, formActions([cancel, test, save]));
  return box;
}

function secretRow(secret) {
  if (secret.set) {
    return settingRow(secret.name, null, el("span", "badge ok", "configured"), true);
  }
  const group = el("span", "ctl-group");
  const input = el("input", "field ctl");
  input.type = "password";
  input.placeholder = "paste value";

  const save = el("button", "btn", "Save");
  save.addEventListener("click", async () => {
    if (!input.value) return;
    try {
      await api("PUT", "/api/secrets", { name: secret.name, value: input.value });
      reload(true);
    } catch (err) { alert(err.message); }
  });

  group.append(input, save);
  return settingRow(secret.name, "Needed before this source can poll.", group, true);
}
