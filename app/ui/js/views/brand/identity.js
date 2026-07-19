import { api, mutate } from "../../core/api.js";
import { el } from "../../core/dom.js";
import { fieldInput } from "../../components/controls.js";
import { formActions, settingRow } from "../../components/rows.js";
import { DESC } from "./fields.js";

const FIELDS = ["name", "tagline", "website", "mascot"];

function readAll(card) {
  const out = {};
  for (const input of card.querySelectorAll("input.field[data-name]")) {
    out[input.dataset.name] = input.value;
  }
  return out;
}

export function identityCard(manifest, onDirty) {
  const card = el("div", "set card");
  const brand = manifest.brand || {};

  for (const name of FIELDS) {
    card.append(settingRow(name, DESC[name], fieldInput(name, {}, brand[name], { ctl: true })));
  }

  const save = el("button", "btn primary", "Save identity");
  save.addEventListener("click", () =>
    mutate(() => api("PUT", "/api/config/identity", readAll(card)), "brand-note"));

  card.append(formActions([save]));

  card.addEventListener("input", onDirty);
  return card;
}
