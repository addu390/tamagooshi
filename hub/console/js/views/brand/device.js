import catalog from "../../catalog.gen.js";
import { el } from "../../core/dom.js";
import { chipPicker, chipValues, fieldInput, selectField, switchControl }
  from "../../components/controls.js";
import { settingRow } from "../../components/rows.js";
import { readFields } from "../../components/schema.js";
import {
  FLASH_HINT, chipsBlock, collectChips, expand, pairs, wireDeviceCard,
} from "./brand.js";

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

export function appearanceCard(manifest, onDirty) {
  const card = el("div", "set card");
  const device = manifest.device || {};

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

  return wireDeviceCard(card, {
    saveLabel: "Save appearance",
    onDirty,
    apply(out) {
      const flat = readFields(card);
      out.theme = { ...out.theme, default: flat.theme_default,
                    enabled: collectChips(device.theme?.enabled, themes) };
      out.typeface = { ...out.typeface, default: flat.typeface_default,
                       enabled: collectChips(device.typeface?.enabled, typefaces) };
    },
  });
}

export function mascotCard(manifest, moods, onDirty) {
  const card = el("div", "set card");
  const device = manifest.device || {};

  card.append(settingRow("Mascot", "Must belong to an enabled pack.",
                         selectField("mascot_default", mascotOptions(),
                                     device.mascot?.default, { ctl: true })));
  card.append(settingRow("Default mood", "How the mascot wakes up.",
                         selectField("mascot_mood", pairs(moods),
                                     device.mascot?.mood || "happy", { ctl: true })));

  const packOptions = pairs(Object.keys(catalog.packs));
  const packs = chipPicker(packOptions, expand(device.mascot?.enabled, packOptions));
  card.append(chipsBlock("Mascot packs", FLASH_HINT, packs));

  return wireDeviceCard(card, {
    saveLabel: "Save mascot",
    onDirty,
    apply(out) {
      const flat = readFields(card);
      out.mascot = { ...out.mascot, default: flat.mascot_default, mood: flat.mascot_mood,
                     enabled: collectChips(device.mascot?.enabled, packs) };
    },
  });
}

export function featuresCard(manifest, onDirty) {
  const card = el("div", "set card");
  const device = manifest.device || {};

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

  return wireDeviceCard(card, {
    saveLabel: "Save features",
    onDirty,
    apply(out) {
      out.games = { enabled: collectChips(device.games?.enabled, games) };
      out.apps = { enabled: collectChips(device.apps?.enabled, apps) };
      out.buddy = { enabled: buddy };
    },
  });
}

export function linkCard(manifest, onDirty) {
  const card = el("div", "set card");
  const device = manifest.device || {};

  card.append(settingRow("Timezone", "UTC offset shown on the clock.",
                         fieldInput("timezone", { default: "+00:00" }, device.timezone,
                                    { ctl: true, slim: true })));

  const linkOptions = catalog.transports.links
    .map(([link]) => [link, `${link} · ${protocolFor(device, link)}`]);
  const links = chipPicker(linkOptions, Object.keys(device.transports || {}));
  card.append(chipsBlock("Transports", FLASH_HINT, links));

  return wireDeviceCard(card, {
    saveLabel: "Save link",
    onDirty,
    apply(out) {
      const flat = readFields(card);
      const picked = chipValues(links);
      if (picked.length) {
        out.transports = Object.fromEntries(
          picked.map((link) => [link, protocolFor(device, link)]));
      } else {
        delete out.transports;
      }
      if (flat.timezone) out.timezone = flat.timezone;
      else delete out.timezone;
    },
  });
}
