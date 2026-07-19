import catalog from "../../catalog.gen.js";
import { api, mutate } from "../../core/api.js";
import { el } from "../../core/dom.js";
import { chipPicker, chipValues, fieldInput, selectField, switchControl }
  from "../../components/controls.js";
import { formActions, settingRow } from "../../components/rows.js";
import { readFields } from "../../components/schema.js";
import { isAll } from "../../wire/blob.js";

const FLASH_HINT = "Applies at the next firmware flash.";

const pairs = (list) => list.map((x) => (Array.isArray(x) ? x : [x, x]));

function expand(enabled, options, fallback = []) {
  if (isAll(enabled)) return options.map(([value]) => value);
  return enabled || fallback;
}

function collectChips(original, picker) {
  const picked = chipValues(picker);
  const chips = [...picker.querySelectorAll(".chip")];
  if (isAll(original) && picked.length === chips.length) return "all";

  const known = new Set(chips.map((chip) => chip.dataset.value));
  const extras = Array.isArray(original) ? original.filter((v) => !known.has(v)) : [];
  return [...picked, ...extras];
}

function customThemes(device) {
  return ((device.theme || {}).custom || []).map((t) => [t.name, t.name]);
}

function mascotOptions() {
  return Object.values(catalog.packs).flat();
}

function protocolFor(device, link) {
  const existing = (device.transports || {})[link];
  return existing || catalog.transports.protocols[link]?.[0]?.[0];
}

function chipsBlock(title, hint, picker) {
  const block = el("div", "set-block");
  block.append(el("div", "set-title", title));
  if (hint) block.append(el("div", "set-desc", hint));
  block.append(picker);
  return block;
}

export function deviceCard(manifest, moods, onDirty) {
  const card = el("div", "set card");
  const device = structuredClone(manifest.device || {});

  const themeOptions = [...pairs(catalog.themes), ...customThemes(device)];
  const themes = chipPicker(themeOptions, expand(device.theme?.enabled, themeOptions));
  card.append(settingRow("Theme", "Default face for the screen.",
                         selectField("theme_default", themeOptions, device.theme?.default,
                                     { ctl: true })));
  card.append(chipsBlock("Themes enabled", "What the owner can pick on the device.", themes));

  const typefaceOptions = pairs(catalog.typefaces);
  const typefaces = chipPicker(typefaceOptions,
                               expand(device.typeface?.enabled, typefaceOptions));
  card.append(settingRow("Typeface", "Default rendering style.",
                         selectField("typeface_default", typefaceOptions,
                                     device.typeface?.default, { ctl: true })));
  card.append(chipsBlock("Typefaces enabled", null, typefaces));

  card.append(settingRow("Mascot", "Must belong to an enabled pack.",
                         selectField("mascot_default", mascotOptions(),
                                     device.mascot?.default, { ctl: true })));
  card.append(settingRow("Default mood", "How the mascot wakes up.",
                         selectField("mascot_mood", pairs(moods),
                                     device.mascot?.mood || "happy", { ctl: true })));

  const packOptions = pairs(Object.keys(catalog.packs));
  const packs = chipPicker(packOptions, expand(device.mascot?.enabled, packOptions));
  card.append(chipsBlock("Mascot packs", FLASH_HINT, packs));

  card.append(settingRow("Timezone", "UTC offset shown on the clock.",
                         fieldInput("timezone", { default: "+00:00" }, device.timezone,
                                    { ctl: true, slim: true })));

  const linkOptions = catalog.transports.links
    .map(([link]) => [link, `${link} · ${protocolFor(device, link)}`]);
  const links = chipPicker(linkOptions, Object.keys(device.transports || {}));
  card.append(chipsBlock("Transports", FLASH_HINT, links));

  const gameOptions = catalog.games.map(([id, desc]) => [id, id, desc]);
  const games = chipPicker(gameOptions, expand(device.games?.enabled, gameOptions));
  card.append(chipsBlock("Games", FLASH_HINT, games));

  const appOptions = pairs(catalog.apps);
  const apps = chipPicker(appOptions, expand(device.apps?.enabled, appOptions));
  card.append(chipsBlock("Apps", FLASH_HINT, apps));

  let buddy = device.buddy?.enabled ?? false;
  const buddySwitch = switchControl(buddy, () => {
    buddy = !buddy;
    buddySwitch.classList.toggle("on", buddy);
    onDirty();
  });
  card.append(settingRow("Buddy", `Voice companion app. ${FLASH_HINT}`, buddySwitch));

  function collect() {
    const flat = readFields(card);
    const out = structuredClone(device);

    out.theme = { ...out.theme, default: flat.theme_default,
                  enabled: collectChips(device.theme?.enabled, themes) };
    out.typeface = { ...out.typeface, default: flat.typeface_default,
                     enabled: collectChips(device.typeface?.enabled, typefaces) };
    out.mascot = { ...out.mascot, default: flat.mascot_default, mood: flat.mascot_mood,
                   enabled: collectChips(device.mascot?.enabled, packs) };
    out.games = { enabled: collectChips(device.games?.enabled, games) };
    out.apps = { enabled: collectChips(device.apps?.enabled, apps) };
    out.buddy = { enabled: buddy };

    const picked = chipValues(links);
    if (picked.length) {
      out.transports = Object.fromEntries(picked.map((link) => [link, protocolFor(device, link)]));
    } else {
      delete out.transports;
    }

    if (flat.timezone) out.timezone = flat.timezone;
    else delete out.timezone;

    return out;
  }

  const save = el("button", "btn primary", "Save device");
  save.addEventListener("click", () =>
    mutate(() => api("PUT", "/api/config/device", collect()), "brand-note"));

  card.append(formActions([save]));

  card.addEventListener("input", onDirty);
  card.addEventListener("click", (e) => {
    if (e.target.closest(".chip-pick")) onDirty();
  });
  return card;
}
