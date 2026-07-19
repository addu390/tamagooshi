import { el } from "../core/dom.js";
import { propType } from "./schema.js";

export function switchControl(on, onToggle) {
  const sw = el("button", "switch" + (on ? " on" : ""));
  sw.addEventListener("click", onToggle);
  return sw;
}

export function segControl(options, active, labelFor, onPick) {
  const seg = el("div", "seg");

  for (const opt of options) {
    const b = el("button", active === opt ? "active" : "", labelFor(opt));
    b.addEventListener("click", () => {
      for (const btn of seg.children) btn.classList.toggle("active", btn === b);
      onPick(opt);
    });
    seg.append(b);
  }

  return seg;
}

export function fieldInput(name, prop, value, { ctl = false, slim = false } = {}) {
  const input = el("input", "field" + (ctl ? " ctl" : "") + (slim ? " slim" : ""));
  const type = propType(prop);
  input.dataset.name = name;
  input.dataset.kind = type === "number" || type === "integer" ? "number" : "text";

  if (prop.default !== undefined && prop.default !== null) {
    input.placeholder = String(prop.default);
  }
  if (value !== undefined && value !== null) input.value = String(value);

  return input;
}

export function labeledControl(label, control) {
  const wrap = el("div", "field-wrap");
  wrap.append(el("span", "field-label", label));
  wrap.append(control);
  return wrap;
}

export function labeledField(name, prop, value) {
  return labeledControl(name.replaceAll("_", " "), fieldInput(name, prop, value));
}

export function pollingControl(values) {
  const group = el("span", "ctl-group");
  group.append(fieldInput("interval_secs", { type: "number", default: 3 },
                          values.interval_secs, { ctl: true, slim: true }));
  group.append(el("span", "field-suffix", "sec"));
  return group;
}

export function selectField(name, options, value, { ctl = false } = {}) {
  const select = el("select", "field" + (ctl ? " ctl" : ""));
  select.dataset.name = name;
  select.dataset.kind = "text";

  for (const [val, label] of options) {
    const opt = el("option", "", label || val);
    opt.value = val;
    if (val === value) opt.selected = true;
    select.append(opt);
  }

  return select;
}

export function chipPicker(options, selected) {
  const wrap = el("div", "chip-pick");

  for (const [value, label, title] of options) {
    const chip = el("button", "chip" + (selected.includes(value) ? " on" : ""), label || value);
    chip.dataset.value = value;
    if (title) chip.title = title;
    chip.addEventListener("click", () => chip.classList.toggle("on"));
    wrap.append(chip);
  }

  return wrap;
}

export function chipValues(picker) {
  return [...picker.querySelectorAll(".chip.on")].map((chip) => chip.dataset.value);
}
