import { el } from "../../core/dom.js";
import { fieldInput, selectField, switchControl } from "../../components/controls.js";
import { settingRow } from "../../components/rows.js";
import { readFields } from "../../components/schema.js";
import { FLASH_HINT, wireDeviceCard } from "./brand.js";

const MONTHS = [
  ["01", "Jan"], ["02", "Feb"], ["03", "Mar"], ["04", "Apr"],
  ["05", "May"], ["06", "Jun"], ["07", "Jul"], ["08", "Aug"],
  ["09", "Sep"], ["10", "Oct"], ["11", "Nov"], ["12", "Dec"],
];

const YEARS = (() => {
  const y = new Date().getFullYear();
  return Array.from({ length: 40 }, (_, i) => {
    const v = String(y - i);
    return [v, v];
  });
})();

function parseJoined(joined) {
  const m = /^(\d{4})-(\d{2})$/.exec(joined || "");
  return m ? { year: m[1], month: m[2] } : { year: "", month: "" };
}

export function personaCard(manifest, onDirty) {
  const card = el("div", "set card");
  const persona = (manifest.device || {}).persona || {};
  const joined = parseJoined(persona.joined);

  card.append(settingRow("Name", `Status bar + about screen. ${FLASH_HINT}`,
                         fieldInput("persona_name", {}, persona.name, { ctl: true })));
  card.append(settingRow("Role", `Shown on the about screen. ${FLASH_HINT}`,
                         fieldInput("persona_role", {}, persona.role, { ctl: true })));

  const joinedCtl = el("span", "ctl-group");
  joinedCtl.append(
    selectField("persona_month", MONTHS, joined.month, { ctl: true, slim: true }),
    selectField("persona_year", YEARS, joined.year, { ctl: true, slim: true }),
  );
  card.append(settingRow("Joined", "Month and year on the about screen.", joinedCtl));
  card.append(settingRow("Avatar",
                         "Brand-relative path to a 6-expression PNG strip.",
                         fieldInput("persona_avatar", { default: "persona.png" },
                                    persona.avatar, { ctl: true })));

  let asMascot = !!persona.mascot;
  const mascotSwitch = switchControl(asMascot, () => {
    asMascot = !asMascot;
    mascotSwitch.classList.toggle("on", asMascot);
    onDirty();
  });
  card.append(settingRow("As mascot",
                         `Offer avatar in the mascot picker. ${FLASH_HINT}`,
                         mascotSwitch));

  return wireDeviceCard(card, {
    saveLabel: "Save persona",
    onDirty,
    apply(out) {
      const flat = readFields(card);
      const name = (flat.persona_name || "").trim();
      const role = (flat.persona_role || "").trim();
      const avatar = (flat.persona_avatar || "").trim();
      const month = flat.persona_month || "";
      const year = flat.persona_year || "";
      if (name && role && avatar && month && year) {
        out.persona = {
          name, role, avatar,
          joined: `${year}-${month}`,
          mascot: asMascot,
        };
      } else {
        delete out.persona;
      }
    },
  });
}
