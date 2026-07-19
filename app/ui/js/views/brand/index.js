import { api, awaitRestart, mutate } from "../../core/api.js";
import { $, el, renderList } from "../../core/dom.js";
import { editHold } from "../../core/hold.js";
import { fieldInput } from "../../components/controls.js";
import { formActions, panelHead, settingRow } from "../../components/rows.js";
import { identityCard } from "./identity.js";
import { deviceCard } from "./device.js";
import { DESC } from "./fields.js";

const guard = editHold();

function activateBrand(id) {
  return mutate(() => api("POST", "/api/brands/activate", { brand: id }), "brand-note");
}

async function deleteBrand(b) {
  const ask = b.overrides_builtin
    ? `Revert "${b.name}" to the built-in version? Your edits are lost.`
    : `Delete "${b.name}"? This cannot be undone.`;
  if (!confirm(ask)) return;

  try {
    const res = await api("DELETE", `/api/brands/${b.id}`);
    if (res.restarting) awaitRestart("brand-note");
    else renderBrands(await api("GET", "/api/brands"));
  } catch (err) {
    alert(err.message);
  }
}

function renderBrands(brands) {
  renderList($("brands"), null, brands, (b) => {
    const row = el("li");
    row.append(el("span", "dot " + (b.active ? "ok" : "")));
    row.append(el("span", "grow", b.name + (b.tagline ? " · " + b.tagline : "")));

    const source = b.source === "user" ? (b.overrides_builtin ? "edited" : "imported") : b.source;
    row.append(el("span", "dim", b.id + " · " + source));

    const download = el("a", "btn", "Download");
    download.href = `/api/brands/${b.id}/export`;
    download.setAttribute("download", `${b.id}.yaml`);
    row.append(download);

    if (!b.active) {
      const use = el("button", "btn", "Use");
      use.addEventListener("click", () => activateBrand(b.id));
      row.append(use);
    }

    if (b.source === "user") {
      const drop = el("button", "btn", b.overrides_builtin ? "Revert" : "Delete");
      drop.addEventListener("click", () => deleteBrand(b));
      row.append(drop);
    }
    return row;
  });
}

const NEW_BRAND_FIELDS = ["id", "name", "tagline"];

function openNewBrand() {
  const panel = $("brand-wizard");
  panel.classList.remove("hide");
  panel.replaceChildren();

  panel.append(panelHead("New brand",
                         "Starts from the template. Shape identity, device and sources here after it activates."));

  const inputs = {};
  const form = el("div", "set");
  for (const name of NEW_BRAND_FIELDS) {
    inputs[name] = fieldInput(name, {}, undefined, { ctl: true });
    form.append(settingRow(name, DESC[name], inputs[name]));
  }
  panel.append(form);

  const cancel = el("button", "btn", "Cancel");
  cancel.addEventListener("click", () => panel.classList.add("hide"));

  const create = el("button", "btn primary", "Create brand");
  create.addEventListener("click", async () => {
    try {
      const { id } = await api("POST", "/api/brands", {
        id: inputs.id.value.trim(),
        name: inputs.name.value.trim(),
        tagline: inputs.tagline.value.trim(),
      });
      panel.classList.add("hide");
      activateBrand(id);
    } catch (err) {
      alert(err.message);
    }
  });

  panel.append(formActions([cancel, create]));
  inputs.id.focus();
}

function render({ brands, manifest, moods }) {
  renderBrands(brands);
  if (guard.isHeld()) return;

  $("brand-identity").replaceChildren(identityCard(manifest, guard.hold));
  $("brand-device").replaceChildren(deviceCard(manifest, moods, guard.hold));
}

async function load() {
  const [brands, manifest, rules] = await Promise.all([
    api("GET", "/api/brands"), api("GET", "/api/config"), api("GET", "/api/rules"),
  ]);
  return { brands, manifest, moods: rules.options.moods };
}

function init() {
  $("brand-new").addEventListener("click", openNewBrand);

  $("brand-file").addEventListener("change", async (e) => {
    const file = e.target.files[0];
    if (!file) return;

    const r = await fetch("/api/brands/import", { method: "POST", body: await file.text() });
    e.target.value = "";

    if (!r.ok) {
      const detail = (await r.json().catch(() => ({}))).detail;
      alert(detail || "import failed");
      return;
    }

    const { id } = await r.json();
    activateBrand(id);
  });
}

export default { id: "brand", title: "Brand", init, load, render };
