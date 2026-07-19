import { el } from "../core/dom.js";

export function settingRow(title, desc, control, mono) {
  const row = el("div", "set-row");
  const info = el("div", "set-info");
  if (!mono) title = title.charAt(0).toUpperCase() + title.slice(1);
  info.append(el("div", "set-title" + (mono ? " mono" : ""), title));
  if (desc) info.append(el("div", "set-desc", desc));
  row.append(info, control);
  return row;
}

export function secHead(title, action) {
  const head = el("div", "sec-head");
  head.append(el("div", "sec-label", title));
  if (action) head.append(action);
  return head;
}

export function typeTile(type, label, big) {
  return el("span", "tile" + (big ? " lg" : ""), (label || type).charAt(0).toUpperCase());
}

export function formActions(buttons, { left = [], divided = false } = {}) {
  const row = el("div", "form-actions" + (divided ? " divided" : ""));
  row.append(...left, el("span", "grow"), ...buttons);
  return row;
}

export function panelHead(title, lead) {
  const head = document.createDocumentFragment();
  head.append(el("div", "wiz-title", title));
  if (lead) head.append(el("p", "wiz-lead", lead));
  return head;
}
