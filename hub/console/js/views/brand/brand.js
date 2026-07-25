import { api, mutate } from "../../core/api.js";
import { el } from "../../core/dom.js";
import { chipValues } from "../../components/controls.js";
import { formActions } from "../../components/rows.js";
import { isAll } from "../../wire/blob.js";

export const FLASH_HINT = "Applies at the next firmware flash.";

export const pairs = (list) => list.map((x) => (Array.isArray(x) ? x : [x, x]));

export function expand(enabled, options, fallback = []) {
  if (isAll(enabled)) return options.map(([value]) => value);
  return enabled || fallback;
}

export function collectChips(original, picker) {
  const picked = chipValues(picker);
  const chips = [...picker.querySelectorAll(".chip")];
  if (isAll(original) && picked.length === chips.length) return "all";

  const known = new Set(chips.map((chip) => chip.dataset.value));
  const extras = Array.isArray(original) ? original.filter((v) => !known.has(v)) : [];
  return [...picked, ...extras];
}

export function chipsBlock(title, hint, picker) {
  const block = el("div", "set-block");
  block.append(el("div", "set-title", title));
  if (hint) block.append(el("div", "set-desc", hint));
  block.append(picker);
  return block;
}

export async function patchDevice(apply) {
  const current = await api("GET", "/api/config");
  const out = structuredClone(current.device || {});
  apply(out);
  return api("PUT", "/api/config/device", out);
}

export function wireDeviceCard(card, { saveLabel, onDirty, apply }) {
  const save = el("button", "btn primary", saveLabel);
  save.addEventListener("click", () =>
    mutate(() => patchDevice(apply), "brand-note"));
  card.append(formActions([save]));
  card.addEventListener("input", onDirty);
  card.addEventListener("change", onDirty);
  card.addEventListener("click", (e) => {
    if (e.target.closest(".chip-pick, .switch")) onDirty();
  });
  return card;
}
